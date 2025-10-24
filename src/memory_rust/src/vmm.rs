// vmm.rs - 虚拟内存管理器
// 管理虚拟地址空间的分配和映射

use crate::arch::addr::VirtAddr;
use crate::paging::{PageTableManager, PAGE_PRESENT, PAGE_WRITABLE};
use crate::lazy_buddy::PhysFrame;

/// 虚拟内存区域类型
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum VmmRegionType {
    KernelCode,      // 内核代码段
    KernelData,      // 内核数据段
    KernelHeap,      // 内核堆
    KernelStack,     // 内核栈
    UserCode,        // 用户代码段
    UserData,        // 用户数据段
    UserHeap,        // 用户堆
    UserStack,       // 用户栈
}

/// 虚拟内存区域标志
#[derive(Debug, Clone, Copy)]
pub struct VmmFlags {
    pub writable: bool,     // 可写
    pub user: bool,         // 用户可访问
    pub executable: bool,   // 可执行
}

impl VmmFlags {
    pub const fn new() -> Self {
        Self {
            writable: false,
            user: false,
            executable: false,
        }
    }

    pub const fn writable(mut self) -> Self {
        self.writable = true;
        self
    }

    pub const fn user(mut self) -> Self {
        self.user = true;
        self
    }

    pub const fn executable(mut self) -> Self {
        self.executable = true;
        self
    }

    // 转换为页表标志位
    pub fn to_page_flags(&self) -> u64 {
        let mut flags = PAGE_PRESENT;
        if self.writable {
            flags |= PAGE_WRITABLE;
        }
        if self.user {
            flags |= 1 << 2; // PAGE_USER
        }
        if !self.executable {
            flags |= 1 << 63; // PAGE_NO_EXECUTE
        }
        flags
    }
}

/// 虚拟内存区域
#[derive(Debug, Clone, Copy)]
pub struct VmmRegion {
    pub start: VirtAddr,
    pub end: VirtAddr,
    pub region_type: VmmRegionType,
    pub flags: VmmFlags,
}

impl VmmRegion {
    pub fn new(start: VirtAddr, end: VirtAddr, region_type: VmmRegionType, flags: VmmFlags) -> Self {
        Self {
            start,
            end,
            region_type,
            flags,
        }
    }

    pub fn size(&self) -> u64 {
        self.end.as_u64() - self.start.as_u64()
    }

    pub fn contains(&self, addr: VirtAddr) -> bool {
        addr.as_u64() >= self.start.as_u64() && addr.as_u64() < self.end.as_u64()
    }

    pub fn overlaps(&self, other: &VmmRegion) -> bool {
        self.start.as_u64() < other.end.as_u64() && self.end.as_u64() > other.start.as_u64()
    }
}

/// 虚拟内存管理器
pub struct VirtualMemoryManager {
    // 虚拟内存区域列表（最多支持32个区域）
    regions: [Option<VmmRegion>; 32],
    region_count: usize,

    // 内核堆分配位置
    kernel_heap_start: VirtAddr,
    kernel_heap_current: VirtAddr,
    kernel_heap_end: VirtAddr,
}

impl VirtualMemoryManager {
    /// 创建新的虚拟内存管理器
    pub const fn new() -> Self {
        Self {
            regions: [None; 32],
            region_count: 0,
            kernel_heap_start: VirtAddr::new(0xFFFFFFFF90000000),  // 内核堆起始地址
            kernel_heap_current: VirtAddr::new(0xFFFFFFFF90000000),
            kernel_heap_end: VirtAddr::new(0xFFFFFFFFA0000000),    // 内核堆结束地址（256MB）
        }
    }

    /// 初始化虚拟内存管理器
    pub fn init(&mut self) -> Result<(), &'static str> {
        // 预定义内核区域
        self.add_region(VmmRegion::new(
            VirtAddr::new(0xFFFFFFFF80000000),  // 内核代码起始
            VirtAddr::new(0xFFFFFFFF90000000),  // 内核代码结束
            VmmRegionType::KernelCode,
            VmmFlags::new().executable(),
        ))?;

        self.add_region(VmmRegion::new(
            VirtAddr::new(0xFFFFFFFF90000000),  // 内核堆起始
            VirtAddr::new(0xFFFFFFFFA0000000),  // 内核堆结束
            VmmRegionType::KernelHeap,
            VmmFlags::new().writable(),
        ))?;

        Ok(())
    }

    /// 添加虚拟内存区域
    pub fn add_region(&mut self, region: VmmRegion) -> Result<(), &'static str> {
        if self.region_count >= self.regions.len() {
            return Err("Too many regions");
        }

        // 检查是否与现有区域重叠
        for i in 0..self.region_count {
            if let Some(existing) = &self.regions[i] {
                if existing.overlaps(&region) {
                    return Err("Region overlaps with existing region");
                }
            }
        }

        self.regions[self.region_count] = Some(region);
        self.region_count += 1;
        Ok(())
    }

    /// 查找包含指定地址的区域
    pub fn find_region(&self, addr: VirtAddr) -> Option<&VmmRegion> {
        for i in 0..self.region_count {
            if let Some(region) = &self.regions[i] {
                if region.contains(addr) {
                    return Some(region);
                }
            }
        }
        None
    }

    /// 分配虚拟内存区域（从内核堆）
    pub fn allocate_kernel_heap(&mut self, size: u64) -> Result<VirtAddr, &'static str> {
        // 页面对齐
        let aligned_size = (size + 0xFFF) & !0xFFF;

        let start = self.kernel_heap_current;
        let end_addr = start.as_u64() + aligned_size;

        if end_addr > self.kernel_heap_end.as_u64() {
            return Err("Kernel heap exhausted");
        }

        self.kernel_heap_current = VirtAddr::new(end_addr);
        Ok(start)
    }

    /// 映射虚拟内存区域到物理内存
    pub fn map_region<F>(
        &self,
        page_table: &mut PageTableManager,
        region: &VmmRegion,
        mut alloc_frame: F,
    ) -> Result<(), &'static str>
    where
        F: FnMut() -> Option<PhysFrame>,
    {
        let page_size = 4096u64;
        let start_page = region.start.as_u64() & !0xFFF;
        let end_page = (region.end.as_u64() + 0xFFF) & !0xFFF;

        let flags = region.flags.to_page_flags();

        for virt_addr in (start_page..end_page).step_by(page_size as usize) {
            // 分配物理页面
            let frame = alloc_frame().ok_or("Failed to allocate physical frame")?;
            let phys_addr = frame.addr();

            // 映射页面
            page_table.map_page(
                VirtAddr::new(virt_addr),
                phys_addr,
                flags,
                &mut alloc_frame,
            )?;
        }

        Ok(())
    }

    /// 取消映射虚拟内存区域
    pub fn unmap_region(
        &self,
        page_table: &mut PageTableManager,
        region: &VmmRegion,
    ) -> Result<(), &'static str> {
        let page_size = 4096u64;
        let start_page = region.start.as_u64() & !0xFFF;
        let end_page = (region.end.as_u64() + 0xFFF) & !0xFFF;

        for virt_addr in (start_page..end_page).step_by(page_size as usize) {
            // 取消映射页面（忽略错误，页面可能未映射）
            let _ = page_table.unmap_page(VirtAddr::new(virt_addr));
        }

        Ok(())
    }

    /// 分配并映射内核堆内存
    pub fn allocate_and_map<F>(
        &mut self,
        page_table: &mut PageTableManager,
        size: u64,
        flags: VmmFlags,
        mut alloc_frame: F,
    ) -> Result<VirtAddr, &'static str>
    where
        F: FnMut() -> Option<PhysFrame>,
    {
        // 分配虚拟地址
        let virt_start = self.allocate_kernel_heap(size)?;
        let virt_end = VirtAddr::new(virt_start.as_u64() + size);

        // 创建临时区域
        let region = VmmRegion::new(
            virt_start,
            virt_end,
            VmmRegionType::KernelHeap,
            flags,
        );

        // 映射到物理内存
        self.map_region(page_table, &region, &mut alloc_frame)?;

        Ok(virt_start)
    }

    /// 获取内核堆使用情况
    pub fn kernel_heap_usage(&self) -> (u64, u64) {
        let used = self.kernel_heap_current.as_u64() - self.kernel_heap_start.as_u64();
        let total = self.kernel_heap_end.as_u64() - self.kernel_heap_start.as_u64();
        (used, total)
    }
}

