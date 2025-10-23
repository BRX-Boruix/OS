//! 物理内存分配器
//! 管理物理页面的分配和释放

use crate::arch::addr::PhysAddr;
use crate::arch::{MemoryRegion, PAGE_SIZE};
use crate::hhdm;
use core::ptr::NonNull;

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

    /// 获取页面帧号
    pub const fn number(&self) -> u64 {
        self.start_address.as_u64() / PAGE_SIZE as u64
    }

    /// 从页面帧号创建页面帧
    pub const fn from_number(number: u64) -> Self {
        Self {
            start_address: PhysAddr::new(number * PAGE_SIZE as u64),
        }
    }
}

/// 空闲页面链表节点
#[repr(C)]
struct FreePageNode {
    next: Option<NonNull<FreePageNode>>,
}

/// 物理内存分配器
pub struct PhysicalAllocator {
    /// 空闲页面链表头
    free_list: Option<NonNull<FreePageNode>>,
    /// 总页面数
    total_pages: usize,
    /// 已分配页面数
    allocated_pages: usize,
    /// 内存区域列表
    memory_regions: [Option<MemoryRegion>; 32],
    /// 内存区域数量
    region_count: usize,
    /// 是否已初始化
    initialized: bool,
}

impl PhysicalAllocator {
    /// 创建新的物理内存分配器
    pub const fn new() -> Self {
        Self {
            free_list: None,
            total_pages: 0,
            allocated_pages: 0,
            memory_regions: [None; 32],
            region_count: 0,
            initialized: false,
        }
    }

    /// 初始化物理内存分配器
    pub fn init(&mut self, memory_map: &[MemoryRegion]) -> Result<(), &'static str> {
        if self.initialized {
            return Err("Physical allocator already initialized");
        }

        // 检查内存映射
        if memory_map.is_empty() {
            return Err("No memory regions provided");
        }

        // 复制内存区域信息
        if memory_map.len() > self.memory_regions.len() {
            return Err("Too many memory regions");
        }

        for (i, region) in memory_map.iter().enumerate() {
            self.memory_regions[i] = Some(*region);
        }
        self.region_count = memory_map.len();

        // 初始化空闲页面链表
        self.init_free_list()?;

        // 检查是否有可用页面
        if self.total_pages == 0 {
            return Err("No available pages after initialization");
        }

        self.initialized = true;
        Ok(())
    }

    /// 初始化空闲页面链表
    fn init_free_list(&mut self) -> Result<(), &'static str> {
        self.free_list = None;
        self.total_pages = 0;

        // 检查HHDM是否已初始化
        if !hhdm::is_initialized() {
            return Err("HHDM not initialized");
        }

        // 遍历所有可用内存区域
        for i in 0..self.region_count {
            if let Some(region) = self.memory_regions[i] {
                if region.is_available() {
                    self.add_region_to_free_list(&region)?;
                }
            }
        }

        Ok(())
    }

    /// 将内存区域添加到空闲链表
    fn add_region_to_free_list(&mut self, region: &MemoryRegion) -> Result<(), &'static str> {
        // 页面对齐起始地址
        let start_addr = PhysAddr::new(region.base_addr).page_align_up().as_u64();
        let end_addr = PhysAddr::new(region.end_addr()).page_align_down().as_u64();

        if start_addr >= end_addr {
            return Ok(()); // 区域太小，无法包含完整页面
        }

        // 计算页面数量
        let page_count = (end_addr - start_addr) / PAGE_SIZE as u64;

        // 跳过内核占用的低地址空间（前16MB）
        let kernel_end = 0x1000000u64; // 16MB
        let actual_start = if start_addr < kernel_end {
            kernel_end
        } else {
            start_addr
        };

        if actual_start >= end_addr {
            return Ok(()); // 整个区域被内核占用
        }

        let actual_page_count = (end_addr - actual_start) / PAGE_SIZE as u64;

        // 将页面添加到空闲链表
        for page_num in 0..actual_page_count {
            let page_addr = actual_start + page_num * PAGE_SIZE as u64;
            self.add_page_to_free_list(PhysAddr::new(page_addr));
        }

        self.total_pages += actual_page_count as usize;
        Ok(())
    }

    /// 将单个页面添加到空闲链表
    fn add_page_to_free_list(&mut self, addr: PhysAddr) {
        // 通过HHDM将物理地址转换为虚拟地址
        let virt_addr = hhdm::phys_to_virt(addr);
        let node_ptr = virt_addr.as_u64() as *mut FreePageNode;

        unsafe {
            let node = &mut *node_ptr;
            node.next = self.free_list;
            self.free_list = NonNull::new(node_ptr);
        }
    }

    /// 分配一个物理页面
    pub fn allocate_frame(&mut self) -> Option<PhysFrame> {
        if !self.initialized {
            return None;
        }

        if let Some(node_ptr) = self.free_list {
            unsafe {
                let node = node_ptr.as_ref();
                self.free_list = node.next;

                // node_ptr是虚拟地址，需要转换回物理地址
                let virt_addr = crate::arch::addr::VirtAddr::new(node_ptr.as_ptr() as u64);
                let frame_addr = hhdm::virt_to_phys(virt_addr)
                    .expect("Failed to convert virtual address to physical");
                self.allocated_pages += 1;

                // 清零页面内容
                self.zero_page(frame_addr);

                Some(PhysFrame::from_start_address(frame_addr))
            }
        } else {
            None
        }
    }

    /// 释放一个物理页面
    pub fn deallocate_frame(&mut self, frame: PhysFrame) {
        if !self.initialized {
            return;
        }

        let addr = frame.start_address();

        // 检查地址是否有效
        if !self.is_valid_frame_address(addr) {
            return;
        }

        // 清零页面内容
        self.zero_page(addr);

        // 添加到空闲链表
        self.add_page_to_free_list(addr);

        if self.allocated_pages > 0 {
            self.allocated_pages -= 1;
        }
    }

    /// 检查页面地址是否有效
    fn is_valid_frame_address(&self, addr: PhysAddr) -> bool {
        if !addr.is_page_aligned() {
            return false;
        }

        // 检查地址是否在有效的内存区域内
        for i in 0..self.region_count {
            if let Some(region) = self.memory_regions[i] {
                if region.is_available() && region.contains(addr.as_u64()) {
                    return true;
                }
            }
        }

        false
    }

    /// 清零页面内容
    fn zero_page(&self, addr: PhysAddr) {
        // 通过HHDM将物理地址转换为虚拟地址
        let virt_addr = hhdm::phys_to_virt(addr);
        let page_ptr = virt_addr.as_u64() as *mut u8;
        unsafe {
            for i in 0..PAGE_SIZE {
                *page_ptr.add(i) = 0;
            }
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

    /// 获取内存使用率（百分比）
    pub fn usage_percent(&self) -> u32 {
        if self.total_pages == 0 {
            0
        } else {
            (self.allocated_pages * 100 / self.total_pages) as u32
        }
    }

    /// 分配连续的物理页面
    pub fn allocate_contiguous_frames(&mut self, count: usize) -> Option<PhysFrame> {
        if count == 0 || count == 1 {
            return self.allocate_frame();
        }

        // 简化实现：对于连续分配，我们尝试找到连续的空闲页面
        // 这里使用简单的线性搜索方法
        self.find_contiguous_free_frames(count)
    }

    /// 查找连续的空闲页面
    fn find_contiguous_free_frames(&mut self, count: usize) -> Option<PhysFrame> {
        // 遍历所有可用内存区域
        for i in 0..self.region_count {
            if let Some(region) = self.memory_regions[i] {
                if region.is_available() {
                    if let Some(frame) = self.find_contiguous_in_region(&region, count) {
                        // 分配找到的连续页面
                        for j in 0..count {
                            let frame_addr = PhysAddr::new(
                                frame.start_address().as_u64() + j as u64 * PAGE_SIZE as u64,
                            );
                            self.remove_from_free_list(frame_addr);
                        }
                        self.allocated_pages += count;
                        return Some(frame);
                    }
                }
            }
        }
        None
    }

    /// 在指定区域查找连续页面
    fn find_contiguous_in_region(&self, region: &MemoryRegion, count: usize) -> Option<PhysFrame> {
        let start_addr = PhysAddr::new(region.base_addr).page_align_up().as_u64();
        let end_addr = PhysAddr::new(region.end_addr()).page_align_down().as_u64();

        let kernel_end = 0x1000000u64; // 16MB
        let actual_start = if start_addr < kernel_end {
            kernel_end
        } else {
            start_addr
        };

        if actual_start >= end_addr {
            return None;
        }

        let total_pages = (end_addr - actual_start) / PAGE_SIZE as u64;

        // 检查是否有足够的连续页面
        if total_pages < count as u64 {
            return None;
        }

        // 简化实现：返回区域开始的页面
        // 实际实现应该检查这些页面是否真的空闲
        Some(PhysFrame::from_start_address(PhysAddr::new(actual_start)))
    }

    /// 从空闲链表移除指定页面
    fn remove_from_free_list(&mut self, addr: PhysAddr) {
        let target_ptr = addr.as_u64() as *mut FreePageNode;

        // 如果是链表头
        if let Some(head_ptr) = self.free_list {
            if head_ptr.as_ptr() == target_ptr {
                unsafe {
                    let head = head_ptr.as_ref();
                    self.free_list = head.next;
                }
                return;
            }
        }

        // 遍历链表查找目标节点
        let mut current = self.free_list;
        while let Some(current_ptr) = current {
            unsafe {
                let current_node = current_ptr.as_ref();
                if let Some(next_ptr) = current_node.next {
                    if next_ptr.as_ptr() == target_ptr {
                        let next_node = next_ptr.as_ref();
                        let new_next = next_node.next;
                        (current_ptr.as_ptr() as *mut FreePageNode)
                            .as_mut()
                            .unwrap()
                            .next = new_next;
                        return;
                    }
                }
                current = current_node.next;
            }
        }
    }

    /// 释放连续的物理页面
    pub fn deallocate_contiguous_frames(&mut self, start_frame: PhysFrame, count: usize) {
        for i in 0..count {
            let frame_addr =
                PhysAddr::new(start_frame.start_address().as_u64() + i as u64 * PAGE_SIZE as u64);
            let frame = PhysFrame::from_start_address(frame_addr);
            self.deallocate_frame(frame);
        }
    }
}
