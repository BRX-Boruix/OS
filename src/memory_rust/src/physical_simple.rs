//! 简化的物理内存分配器 - 阶段2专用
//! 使用位图管理空闲页面，避免直接访问物理地址

use crate::arch::addr::PhysAddr;
use crate::arch::{MemoryRegion, PAGE_SIZE};

/// 物理页面帧
#[repr(transparent)]
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub struct PhysFrame {
    start_address: PhysAddr,
}

impl PhysFrame {
    /// 从物理地址创建页面帧
    pub const fn from_start_address(address: PhysAddr) -> Self {
        Self {
            start_address: address,
        }
    }

    /// 获取起始地址
    pub const fn start_address(&self) -> PhysAddr {
        self.start_address
    }
}

/// 简化的物理内存分配器
/// 使用位图管理最多32768个页面（128MB内存）
pub struct PhysicalAllocator {
    /// 起始物理地址
    start_addr: u64,
    /// 总页面数
    total_pages: usize,
    /// 已分配页面数
    allocated_pages: usize,
    /// 空闲页面位图（每位代表一个页面，1=空闲，0=已分配）
    bitmap: [u64; 512],  // 512 * 64 = 32768个页面 = 128MB
    /// 是否已初始化
    initialized: bool,
}

impl PhysicalAllocator {
    /// 创建新的物理内存分配器
    pub const fn new() -> Self {
        Self {
            start_addr: 0,
            total_pages: 0,
            allocated_pages: 0,
            bitmap: [0; 512],
            initialized: false,
        }
    }

    /// 初始化物理内存分配器
    pub fn init(&mut self, memory_map: &[MemoryRegion]) -> Result<(), &'static str> {
        if self.initialized {
            return Err("Physical allocator already initialized");
        }

        if memory_map.is_empty() {
            return Err("No memory regions provided");
        }

        // 找到第一个可用的内存区域
        let mut found_region = None;
        for region in memory_map {
            if region.is_available() {
                found_region = Some(region);
                break;
            }
        }

        let region = match found_region {
            Some(r) => r,
            None => return Err("No available memory region found"),
        };

        // 页面对齐起始地址，跳过低16MB（内核占用）
        let kernel_end = 0x1000000u64; // 16MB
        let start_addr = PhysAddr::new(region.base_addr).page_align_up().as_u64();
        let actual_start = if start_addr < kernel_end {
            kernel_end
        } else {
            start_addr
        };

        let end_addr = PhysAddr::new(region.end_addr()).page_align_down().as_u64();

        if actual_start >= end_addr {
            return Err("No usable memory after kernel");
        }

        // 计算可用页面数
        let available_pages = ((end_addr - actual_start) / PAGE_SIZE as u64) as usize;
        let max_pages = self.bitmap.len() * 64; // 最大支持的页面数

        let pages_to_use = if available_pages > max_pages {
            max_pages
        } else {
            available_pages
        };

        self.start_addr = actual_start;
        self.total_pages = pages_to_use;
        self.allocated_pages = 0;

        // 初始化位图：所有位设为1（空闲）
        let full_u64s = pages_to_use / 64;
        let remaining_bits = pages_to_use % 64;

        for i in 0..full_u64s {
            self.bitmap[i] = !0u64; // 全1
        }

        if remaining_bits > 0 {
            self.bitmap[full_u64s] = (1u64 << remaining_bits) - 1;
        }

        self.initialized = true;
        Ok(())
    }

    /// 分配一个物理页面
    pub fn allocate_frame(&mut self) -> Option<PhysFrame> {
        if !self.initialized {
            return None;
        }

        // 查找第一个空闲页面
        for (i, &word) in self.bitmap.iter().enumerate() {
            if word == 0 {
                continue; // 这个u64没有空闲位
            }

            // 找到第一个为1的位
            let bit_pos = word.trailing_zeros() as usize;
            let page_num = i * 64 + bit_pos;

            if page_num >= self.total_pages {
                break;
            }

            // 标记为已分配（清除该位）
            self.bitmap[i] &= !(1u64 << bit_pos);
            self.allocated_pages += 1;

            // 计算物理地址
            let phys_addr = self.start_addr + (page_num as u64 * PAGE_SIZE as u64);
            return Some(PhysFrame::from_start_address(PhysAddr::new(phys_addr)));
        }

        None // 没有空闲页面
    }

    /// 释放一个物理页面
    pub fn deallocate_frame(&mut self, frame: PhysFrame) {
        if !self.initialized {
            return;
        }

        let addr = frame.start_address().as_u64();

        // 检查地址是否在有效范围内
        if addr < self.start_addr {
            return;
        }

        let offset = addr - self.start_addr;
        if offset % PAGE_SIZE as u64 != 0 {
            return; // 未对齐
        }

        let page_num = (offset / PAGE_SIZE as u64) as usize;
        if page_num >= self.total_pages {
            return; // 超出范围
        }

        let word_idx = page_num / 64;
        let bit_pos = page_num % 64;

        // 标记为空闲（设置该位为1）
        self.bitmap[word_idx] |= 1u64 << bit_pos;

        if self.allocated_pages > 0 {
            self.allocated_pages -= 1;
        }
    }

    /// 获取总页面数
    pub const fn total_pages(&self) -> usize {
        self.total_pages
    }

    /// 获取已分配页面数
    pub const fn allocated_pages(&self) -> usize {
        self.allocated_pages
    }

    /// 获取空闲页面数
    pub const fn free_pages(&self) -> usize {
        self.total_pages - self.allocated_pages
    }
}

