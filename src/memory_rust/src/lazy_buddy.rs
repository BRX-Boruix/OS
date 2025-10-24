//! 懒加载伙伴分配器（Lazy Buddy Allocator）
//! 
//! 核心思想：
//! 1. 使用伙伴系统的元数据结构和算法
//! 2. 采用懒加载策略：初始化时不访问物理内存
//! 3. 按需初始化：分配时才将页面纳入伙伴系统管理
//! 
//! 优势：
//! - 初始化快速（只设置元数据）
//! - 支持大内存（无限制）
//! - 高效分配（O(log n)）
//! - 自动合并碎片

use crate::arch::addr::PhysAddr;
use crate::arch::{MemoryRegion, PAGE_SIZE};
use crate::hhdm;
use core::slice;

/// 最大order数（支持到2MB大页）
pub const MAX_ORDER: usize = 10;  // order 9 = 512页 = 2MB

/// Order 9 = 512页 = 2MB
pub const ORDER_2M: usize = 9;

/// 无效索引标记
const INVALID_INDEX: usize = usize::MAX;

/// 伙伴帧元数据
#[repr(C)]
#[derive(Clone, Copy)]
pub struct BuddyFrame {
    /// 当前块的order（大小 = 2^order 页）
    pub order: u8,
    /// 是否空闲
    pub is_free: bool,
    /// 是否已初始化（加入伙伴系统管理）
    pub is_initialized: bool,
    /// 空闲链表中的下一个索引
    pub next: usize,
}

impl BuddyFrame {
    pub const fn new() -> Self {
        Self {
            order: 0,
            is_free: false,
            is_initialized: false,
            next: INVALID_INDEX,
        }
    }
}

/// 未初始化的内存区域
#[derive(Clone, Copy)]
struct UninitRegion {
    start_frame: usize,
    end_frame: usize,
}

/// 物理页面帧
#[repr(transparent)]
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub struct PhysFrame {
    start_address: PhysAddr,
}

impl PhysFrame {
    pub const fn from_start_address(address: PhysAddr) -> Self {
        Self { start_address: address }
    }

    pub const fn start_address(&self) -> PhysAddr {
        self.start_address
    }

    // 别名方法，与start_address()相同
    pub const fn addr(&self) -> PhysAddr {
        self.start_address
    }
}

/// 懒加载伙伴分配器
pub struct LazyBuddyAllocator {
    /// 元数据数组（每个物理页一个）
    frames: Option<&'static mut [BuddyFrame]>,
    
    /// 伙伴系统空闲链表头（按order索引）
    free_lists: [usize; MAX_ORDER],
    
    /// 未初始化区域列表
    uninit_regions: [Option<UninitRegion>; 32],
    uninit_count: usize,
    current_region: usize,
    current_offset: usize,
    
    /// 统计信息
    total_frames: usize,
    allocated_frames: usize,
    initialized_frames: usize,
    
    /// 是否已初始化
    initialized: bool,
}

impl LazyBuddyAllocator {
    /// 创建新的分配器
    pub const fn new() -> Self {
        Self {
            frames: None,
            free_lists: [INVALID_INDEX; MAX_ORDER],
            uninit_regions: [None; 32],
            uninit_count: 0,
            current_region: 0,
            current_offset: 0,
            total_frames: 0,
            allocated_frames: 0,
            initialized_frames: 0,
            initialized: false,
        }
    }

    /// 初始化分配器
    /// 
    /// 关键：只初始化元数据数组，不访问物理页面！
    pub fn init(&mut self, memory_map: &[MemoryRegion]) -> Result<(), &'static str> {
        if self.initialized {
            return Err("Allocator already initialized");
        }

        if memory_map.is_empty() {
            return Err("No memory regions provided");
        }

        // 检查HHDM是否已初始化
        if !hhdm::is_initialized() {
            return Err("HHDM not initialized");
        }

        // 1. 计算总帧数
        self.total_frames = 0;
        for region in memory_map {
            if region.is_available() {
                let start = (region.base_addr + PAGE_SIZE as u64 - 1) / PAGE_SIZE as u64;
                let end = (region.base_addr + region.length) / PAGE_SIZE as u64;
                if end > start {
                    self.total_frames += (end - start) as usize;
                }
            }
        }

        if self.total_frames == 0 {
            return Err("No available memory");
        }

        // 2. 找到足够大的区域放置元数据
        let metadata_size = self.total_frames * core::mem::size_of::<BuddyFrame>();
        let metadata_frames = (metadata_size + PAGE_SIZE - 1) / PAGE_SIZE;

        let mut metadata_phys = 0u64;
        let mut metadata_region_idx = 0;

        for (idx, region) in memory_map.iter().enumerate() {
            if !region.is_available() {
                continue;
            }
            
            let region_frames = region.length / PAGE_SIZE as u64;
            if region_frames >= metadata_frames as u64 {
                metadata_phys = region.base_addr;
                metadata_region_idx = idx;
                break;
            }
        }

        if metadata_phys == 0 {
            return Err("No region large enough for metadata");
        }

        // 3. 通过HHDM映射元数据区域
        let metadata_virt = hhdm::phys_to_virt(PhysAddr::new(metadata_phys));
        
        // 4. 初始化元数据数组（在虚拟内存中，安全）
        self.frames = Some(unsafe {
            let ptr = metadata_virt.as_u64() as *mut BuddyFrame;
            
            // 清零元数据区域
            core::ptr::write_bytes(ptr, 0, self.total_frames);
            
            slice::from_raw_parts_mut(ptr, self.total_frames)
        });

        // 5. 构建未初始化区域列表
        self.uninit_count = 0;
        let mut frame_idx = 0;

        for (idx, region) in memory_map.iter().enumerate() {
            if !region.is_available() {
                continue;
            }

            let start_frame = (region.base_addr + PAGE_SIZE as u64 - 1) / PAGE_SIZE as u64;
            let end_frame = (region.base_addr + region.length) / PAGE_SIZE as u64;

            let mut actual_start = start_frame as usize;
            let actual_end = end_frame as usize;

            // 如果是放置元数据的区域，跳过元数据占用的帧
            if idx == metadata_region_idx {
                actual_start += metadata_frames;
            }

            if actual_start < actual_end {
                self.uninit_regions[self.uninit_count] = Some(UninitRegion {
                    start_frame: frame_idx + (actual_start - start_frame as usize),
                    end_frame: frame_idx + (actual_end - start_frame as usize),
                });
                self.uninit_count += 1;
            }

            frame_idx += actual_end - start_frame as usize;
        }

        // 6. 设置当前分配位置
        extern "C" {
            fn serial_puts(s: *const u8);
            fn serial_put_dec(value: u64);
        }
        
        if let Some(region) = self.uninit_regions[0] {
            self.current_region = 0;
            self.current_offset = region.start_frame;
            
            unsafe {
                serial_puts(b"[RUST-INIT] Uninit regions: \0".as_ptr());
                serial_put_dec(self.uninit_count as u64);
                serial_puts(b"\n[RUST-INIT] First region: \0".as_ptr());
                serial_put_dec(region.start_frame as u64);
                serial_puts(b" - \0".as_ptr());
                serial_put_dec(region.end_frame as u64);
                serial_puts(b"\n\0".as_ptr());
            }
        } else {
            unsafe {
                serial_puts(b"[RUST-INIT] ERROR: No uninit regions!\n\0".as_ptr());
            }
        }

        unsafe {
            serial_puts(b"[RUST-INIT] Total frames: \0".as_ptr());
            serial_put_dec(self.total_frames as u64);
            serial_puts(b"\n\0".as_ptr());
        }

        self.initialized = true;
        Ok(())
    }

    /// 分配指定order的页面
    pub fn allocate_order(&mut self, order: usize) -> Option<PhysFrame> {
        // 外部函数用于串口调试
        extern "C" {
            fn serial_puts(s: *const u8);
        }
        
        if !self.initialized {
            unsafe { serial_puts(b"[RUST-ALLOC] Not initialized\n\0".as_ptr()); }
            return None;
        }
        
        if order >= MAX_ORDER {
            unsafe { serial_puts(b"[RUST-ALLOC] Order too large\n\0".as_ptr()); }
            return None;
        }
        
        unsafe { serial_puts(b"[RUST-ALLOC] Trying buddy list...\n\0".as_ptr()); }

        // 策略1: 先从伙伴系统分配
        if let Some(frame_idx) = self.buddy_alloc_from_list(order) {
            unsafe { serial_puts(b"[RUST-ALLOC] Allocated from buddy list\n\0".as_ptr()); }
            let addr = PhysAddr::new((frame_idx * PAGE_SIZE) as u64);
            return Some(PhysFrame::from_start_address(addr));
        }

        unsafe { serial_puts(b"[RUST-ALLOC] Trying lazy alloc...\n\0".as_ptr()); }

        // 策略2: 从未初始化区域懒分配
        if let Some(frame_idx) = self.lazy_alloc_fresh(order) {
            unsafe { serial_puts(b"[RUST-ALLOC] Allocated from lazy pool\n\0".as_ptr()); }
            let addr = PhysAddr::new((frame_idx * PAGE_SIZE) as u64);
            return Some(PhysFrame::from_start_address(addr));
        }

        unsafe { serial_puts(b"[RUST-ALLOC] No memory available\n\0".as_ptr()); }
        None
    }

    /// 分配单个页面（order 0）
    pub fn allocate_frame(&mut self) -> Option<PhysFrame> {
        self.allocate_order(0)
    }

    /// 从伙伴系统的空闲链表分配
    fn buddy_alloc_from_list(&mut self, order: usize) -> Option<usize> {
        // 向上查找有空闲块的order
        let mut current_order = order;
        while current_order < MAX_ORDER {
            if self.free_lists[current_order] != INVALID_INDEX {
                let idx = self.free_lists[current_order];
                
                // 从链表移除
                {
                    let frames = self.frames.as_mut()?;
                    self.free_lists[current_order] = frames[idx].next;
                    frames[idx].is_free = false;
                    frames[idx].next = INVALID_INDEX;
                }
                
                // 分裂到目标order（释放frames的借用）
                while current_order > order {
                    current_order -= 1;
                    let buddy_idx = idx + (1 << current_order);
                    
                    if buddy_idx < self.total_frames {
                        self.add_to_free_list(buddy_idx, current_order);
                    }
                }
                
                // 设置order
                if let Some(frames) = self.frames.as_mut() {
                    frames[idx].order = order as u8;
                }
                
                self.allocated_frames += 1 << order;
                return Some(idx);
            }
            current_order += 1;
        }

        None
    }

    /// 从未初始化区域懒分配
    fn lazy_alloc_fresh(&mut self, order: usize) -> Option<usize> {
        let needed_frames = 1 << order;

        while self.current_region < self.uninit_count {
            if let Some(region) = self.uninit_regions[self.current_region] {
                let available = region.end_frame - self.current_offset;

                if available >= needed_frames {
                    let frame_idx = self.current_offset;
                    self.current_offset += needed_frames;

                    // 标记为已初始化
                    if let Some(frames) = self.frames.as_mut() {
                        for i in 0..needed_frames {
                            if frame_idx + i < self.total_frames {
                                frames[frame_idx + i].is_initialized = true;
                                frames[frame_idx + i].order = order as u8;
                                frames[frame_idx + i].is_free = false;
                            }
                        }
                    }

                    self.allocated_frames += needed_frames;
                    self.initialized_frames += needed_frames;
                    return Some(frame_idx);
                }
            }

            // 切换到下一个区域
            self.current_region += 1;
            if let Some(region) = self.uninit_regions[self.current_region] {
                self.current_offset = region.start_frame;
            }
        }

        None
    }

    /// 释放页面并尝试与伙伴合并
    pub fn deallocate_frame(&mut self, frame: PhysFrame) {
        self.deallocate_order(frame, 0);
    }

    /// 释放指定order的页面
    pub fn deallocate_order(&mut self, frame: PhysFrame, order: usize) {
        if !self.initialized {
            return;
        }

        let frame_idx = (frame.start_address().as_u64() / PAGE_SIZE as u64) as usize;
        
        if frame_idx >= self.total_frames {
            return;
        }

        let frames = match self.frames.as_mut() {
            Some(f) => f,
            None => return,
        };

        if !frames[frame_idx].is_initialized {
            return;
        }

        // 伙伴系统释放并合并
        self.buddy_free_and_merge(frame_idx, order);
        
        self.allocated_frames -= 1 << order;
    }

    /// 伙伴释放并合并
    fn buddy_free_and_merge(&mut self, mut idx: usize, mut order: usize) {
        // 尝试与伙伴合并
        loop {
            if order >= MAX_ORDER - 1 {
                break;
            }
            
            let buddy_idx = idx ^ (1 << order);

            // 检查伙伴是否可以合并
            if buddy_idx >= self.total_frames {
                break;
            }

            // 检查伙伴状态（临时借用）
            let can_merge = {
                let frames = match self.frames.as_ref() {
                    Some(f) => f,
                    None => break,
                };
                
                let buddy = frames[buddy_idx];
                buddy.is_free && buddy.is_initialized && buddy.order as usize == order
            };
            
            if !can_merge {
                break;
            }

            // 从空闲链表移除伙伴
            if !self.remove_from_free_list(buddy_idx, order) {
                break;
            }

            // 合并：使用较小的索引
            if buddy_idx < idx {
                idx = buddy_idx;
            }
            order += 1;
        }

        // 添加到空闲链表
        self.add_to_free_list(idx, order);
    }

    /// 添加到空闲链表
    fn add_to_free_list(&mut self, idx: usize, order: usize) {
        if let Some(frames) = self.frames.as_mut() {
            if idx < self.total_frames && order < MAX_ORDER {
                frames[idx].is_free = true;
                frames[idx].order = order as u8;
                frames[idx].is_initialized = true;
                frames[idx].next = self.free_lists[order];
                self.free_lists[order] = idx;
            }
        }
    }

    /// 从空闲链表移除
    fn remove_from_free_list(&mut self, target_idx: usize, order: usize) -> bool {
        if order >= MAX_ORDER {
            return false;
        }

        let frames = match self.frames.as_mut() {
            Some(f) => f,
            None => return false,
        };

        let mut prev = INVALID_INDEX;
        let mut current = self.free_lists[order];

        while current != INVALID_INDEX {
            if current == target_idx {
                if prev == INVALID_INDEX {
                    self.free_lists[order] = frames[current].next;
                } else {
                    frames[prev].next = frames[current].next;
                }
                frames[current].next = INVALID_INDEX;
                frames[current].is_free = false;
                return true;
            }
            prev = current;
            current = frames[current].next;
        }

        false
    }

    /// 获取总页面数
    pub const fn total_pages(&self) -> usize {
        self.total_frames
    }

    /// 获取已分配页面数
    pub const fn allocated_pages(&self) -> usize {
        self.allocated_frames
    }

    /// 获取已初始化页面数
    pub const fn initialized_pages(&self) -> usize {
        self.initialized_frames
    }

    /// 获取空闲页面数
    pub const fn free_pages(&self) -> usize {
        self.total_frames - self.allocated_frames
    }
}

