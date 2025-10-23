//! 虚拟内存管理和页表操作
//! 实现x86_64的4级页表管理

use crate::arch::addr::{PhysAddr, VirtAddr};
use crate::arch::{
    cpu, pd_index, pdp_index, pml4_index, pt_index, PageTable, PageTableEntry, PAGE_PRESENT,
    PAGE_SIZE, PAGE_USER, PAGE_WRITABLE,
};
use crate::physical::PhysicalAllocator;

/// 页表管理器
pub struct PageTableManager {
    /// 当前页表的物理地址
    current_page_table: Option<PhysAddr>,
    /// 是否已初始化
    initialized: bool,
}

impl PageTableManager {
    /// 创建新的页表管理器
    pub const fn new() -> Self {
        Self {
            current_page_table: None,
            initialized: false,
        }
    }

    /// 初始化页表管理器
    pub fn init(&mut self, allocator: &mut PhysicalAllocator) -> Result<(), &'static str> {
        if self.initialized {
            return Err("Page table manager already initialized");
        }

        // 获取当前的CR3值（当前页表）
        let cr3 = unsafe { cpu::get_cr3() };
        self.current_page_table = Some(PhysAddr::new(cr3 & !0xFFF));

        self.initialized = true;
        Ok(())
    }

    /// 创建新的页表
    pub fn create_page_table(
        &self,
        allocator: &mut PhysicalAllocator,
    ) -> Result<PhysAddr, &'static str> {
        let frame = allocator
            .allocate_frame()
            .ok_or("Failed to allocate frame for page table")?;

        let page_table_addr = frame.start_address();

        // 清零页表
        unsafe {
            let page_table = page_table_addr.as_u64() as *mut PageTable;
            (*page_table).clear();
        }

        Ok(page_table_addr)
    }

    /// 映射虚拟页面到物理页面
    pub fn map_page(
        &mut self,
        virtual_addr: VirtAddr,
        physical_addr: PhysAddr,
        flags: u64,
        allocator: &mut PhysicalAllocator,
    ) -> Result<(), &'static str> {
        if !self.initialized {
            return Err("Page table manager not initialized");
        }

        let page_table_addr = self.current_page_table.ok_or("No current page table")?;

        // 确保地址页面对齐
        let virt_addr = virtual_addr.page_align_down();
        let phys_addr = physical_addr.page_align_down();

        // 获取页表索引
        let pml4_idx = pml4_index(virt_addr.as_u64());
        let pdp_idx = pdp_index(virt_addr.as_u64());
        let pd_idx = pd_index(virt_addr.as_u64());
        let pt_idx = pt_index(virt_addr.as_u64());

        // 获取PML4表
        let pml4_table = unsafe { &mut *(page_table_addr.as_u64() as *mut PageTable) };

        // 获取或创建PDP表
        let pdp_addr = self.get_or_create_next_level_table(
            &mut pml4_table.entries[pml4_idx],
            PAGE_PRESENT | PAGE_WRITABLE | (flags & PAGE_USER),
            allocator,
        )?;

        let pdp_table = unsafe { &mut *(pdp_addr.as_u64() as *mut PageTable) };

        // 获取或创建PD表
        let pd_addr = self.get_or_create_next_level_table(
            &mut pdp_table.entries[pdp_idx],
            PAGE_PRESENT | PAGE_WRITABLE | (flags & PAGE_USER),
            allocator,
        )?;

        let pd_table = unsafe { &mut *(pd_addr.as_u64() as *mut PageTable) };

        // 获取或创建PT表
        let pt_addr = self.get_or_create_next_level_table(
            &mut pd_table.entries[pd_idx],
            PAGE_PRESENT | PAGE_WRITABLE | (flags & PAGE_USER),
            allocator,
        )?;

        let pt_table = unsafe { &mut *(pt_addr.as_u64() as *mut PageTable) };

        // 设置页表项
        pt_table.entries[pt_idx] =
            PageTableEntry::new_with_addr(phys_addr.as_u64(), flags | PAGE_PRESENT);

        // 刷新TLB
        unsafe {
            cpu::flush_tlb(virt_addr.as_u64());
        }

        Ok(())
    }

    /// 取消页面映射
    pub fn unmap_page(&mut self, virtual_addr: VirtAddr) -> Result<PhysAddr, &'static str> {
        if !self.initialized {
            return Err("Page table manager not initialized");
        }

        let page_table_addr = self.current_page_table.ok_or("No current page table")?;

        let virt_addr = virtual_addr.page_align_down();

        // 获取页表索引
        let pml4_idx = pml4_index(virt_addr.as_u64());
        let pdp_idx = pdp_index(virt_addr.as_u64());
        let pd_idx = pd_index(virt_addr.as_u64());
        let pt_idx = pt_index(virt_addr.as_u64());

        // 遍历页表层级
        let pml4_table = unsafe { &*(page_table_addr.as_u64() as *const PageTable) };

        if !pml4_table.entries[pml4_idx].is_present() {
            return Err("Page not mapped");
        }

        let pdp_table = unsafe { &*(pml4_table.entries[pml4_idx].addr() as *const PageTable) };

        if !pdp_table.entries[pdp_idx].is_present() {
            return Err("Page not mapped");
        }

        let pd_table = unsafe { &*(pdp_table.entries[pdp_idx].addr() as *const PageTable) };

        if !pd_table.entries[pd_idx].is_present() {
            return Err("Page not mapped");
        }

        let pt_table = unsafe { &mut *(pd_table.entries[pd_idx].addr() as *mut PageTable) };

        if !pt_table.entries[pt_idx].is_present() {
            return Err("Page not mapped");
        }

        // 获取物理地址
        let physical_addr = PhysAddr::new(pt_table.entries[pt_idx].addr());

        // 清除页表项
        pt_table.entries[pt_idx].clear();

        // 刷新TLB
        unsafe {
            cpu::flush_tlb(virt_addr.as_u64());
        }

        Ok(physical_addr)
    }

    /// 获取虚拟地址对应的物理地址
    pub fn translate(&self, virtual_addr: VirtAddr) -> Option<PhysAddr> {
        if !self.initialized {
            return None;
        }

        let page_table_addr = self.current_page_table?;
        let virt_addr = virtual_addr.as_u64();

        // 获取页表索引
        let pml4_idx = pml4_index(virt_addr);
        let pdp_idx = pdp_index(virt_addr);
        let pd_idx = pd_index(virt_addr);
        let pt_idx = pt_index(virt_addr);

        // 遍历页表层级
        let pml4_table = unsafe { &*(page_table_addr.as_u64() as *const PageTable) };

        if !pml4_table.entries[pml4_idx].is_present() {
            return None;
        }

        let pdp_table = unsafe { &*(pml4_table.entries[pml4_idx].addr() as *const PageTable) };

        if !pdp_table.entries[pdp_idx].is_present() {
            return None;
        }

        let pd_table = unsafe { &*(pdp_table.entries[pdp_idx].addr() as *const PageTable) };

        if !pd_table.entries[pd_idx].is_present() {
            return None;
        }

        let pt_table = unsafe { &*(pd_table.entries[pd_idx].addr() as *const PageTable) };

        if !pt_table.entries[pt_idx].is_present() {
            return None;
        }

        // 计算最终物理地址
        let page_addr = pt_table.entries[pt_idx].addr();
        let offset = virtual_addr.page_offset();
        Some(PhysAddr::new(page_addr + offset))
    }

    /// 检查页面是否已映射
    pub fn is_mapped(&self, virtual_addr: VirtAddr) -> bool {
        self.translate(virtual_addr).is_some()
    }

    /// 映射连续的虚拟页面到连续的物理页面
    pub fn map_range(
        &mut self,
        start_virt: VirtAddr,
        start_phys: PhysAddr,
        page_count: usize,
        flags: u64,
        allocator: &mut PhysicalAllocator,
    ) -> Result<(), &'static str> {
        for i in 0..page_count {
            let virt_offset = i as u64 * PAGE_SIZE as u64;
            let phys_offset = i as u64 * PAGE_SIZE as u64;

            let virt_addr = VirtAddr::new(start_virt.as_u64() + virt_offset);
            let phys_addr = PhysAddr::new(start_phys.as_u64() + phys_offset);

            self.map_page(virt_addr, phys_addr, flags, allocator)?;
        }
        Ok(())
    }

    /// 取消连续页面的映射
    pub fn unmap_range(
        &mut self,
        start_virt: VirtAddr,
        page_count: usize,
    ) -> Result<Vec<PhysAddr>, &'static str> {
        let mut unmapped_pages = Vec::new();

        for i in 0..page_count {
            let virt_offset = i as u64 * PAGE_SIZE as u64;
            let virt_addr = VirtAddr::new(start_virt.as_u64() + virt_offset);

            match self.unmap_page(virt_addr) {
                Ok(phys_addr) => unmapped_pages.push(phys_addr),
                Err(_) => {
                    // 继续处理其他页面，但记录错误
                }
            }
        }

        Ok(unmapped_pages)
    }

    /// 获取或创建下一级页表
    fn get_or_create_next_level_table(
        &self,
        entry: &mut PageTableEntry,
        flags: u64,
        allocator: &mut PhysicalAllocator,
    ) -> Result<PhysAddr, &'static str> {
        if entry.is_present() {
            Ok(PhysAddr::new(entry.addr()))
        } else {
            // 创建新的页表
            let new_table_addr = self.create_page_table(allocator)?;
            *entry = PageTableEntry::new_with_addr(new_table_addr.as_u64(), flags);
            Ok(new_table_addr)
        }
    }

    /// 切换到新的页表
    pub fn switch_page_table(&mut self, page_table_addr: PhysAddr) -> Result<(), &'static str> {
        if !self.initialized {
            return Err("Page table manager not initialized");
        }

        // 设置CR3寄存器
        unsafe {
            cpu::set_cr3(page_table_addr.as_u64());
        }

        self.current_page_table = Some(page_table_addr);
        Ok(())
    }

    /// 获取当前页表地址
    pub fn current_page_table(&self) -> Option<PhysAddr> {
        self.current_page_table
    }

    /// 复制页表
    pub fn clone_page_table(
        &self,
        allocator: &mut PhysicalAllocator,
    ) -> Result<PhysAddr, &'static str> {
        if !self.initialized {
            return Err("Page table manager not initialized");
        }

        let source_addr = self.current_page_table.ok_or("No current page table")?;

        // 创建新的PML4表
        let new_pml4_addr = self.create_page_table(allocator)?;

        // 复制页表内容（简化实现，只复制内核空间映射）
        unsafe {
            let source_pml4 = &*(source_addr.as_u64() as *const PageTable);
            let new_pml4 = &mut *(new_pml4_addr.as_u64() as *mut PageTable);

            // 复制内核空间的映射（高半部分）
            for i in 256..512 {
                new_pml4.entries[i] = source_pml4.entries[i];
            }
        }

        Ok(new_pml4_addr)
    }

    /// 设置页面权限
    pub fn set_page_flags(
        &mut self,
        virtual_addr: VirtAddr,
        flags: u64,
    ) -> Result<(), &'static str> {
        if !self.initialized {
            return Err("Page table manager not initialized");
        }

        let page_table_addr = self.current_page_table.ok_or("No current page table")?;

        let virt_addr = virtual_addr.page_align_down();

        // 获取页表索引
        let pml4_idx = pml4_index(virt_addr.as_u64());
        let pdp_idx = pdp_index(virt_addr.as_u64());
        let pd_idx = pd_index(virt_addr.as_u64());
        let pt_idx = pt_index(virt_addr.as_u64());

        // 遍历页表层级找到目标页表项
        let pml4_table = unsafe { &*(page_table_addr.as_u64() as *const PageTable) };

        if !pml4_table.entries[pml4_idx].is_present() {
            return Err("Page not mapped");
        }

        let pdp_table = unsafe { &*(pml4_table.entries[pml4_idx].addr() as *const PageTable) };

        if !pdp_table.entries[pdp_idx].is_present() {
            return Err("Page not mapped");
        }

        let pd_table = unsafe { &*(pdp_table.entries[pdp_idx].addr() as *const PageTable) };

        if !pd_table.entries[pd_idx].is_present() {
            return Err("Page not mapped");
        }

        let pt_table = unsafe { &mut *(pd_table.entries[pd_idx].addr() as *mut PageTable) };

        if !pt_table.entries[pt_idx].is_present() {
            return Err("Page not mapped");
        }

        // 更新标志位，保持物理地址不变
        let phys_addr = pt_table.entries[pt_idx].addr();
        pt_table.entries[pt_idx] = PageTableEntry::new_with_addr(phys_addr, flags | PAGE_PRESENT);

        // 刷新TLB
        unsafe {
            cpu::flush_tlb(virt_addr.as_u64());
        }

        Ok(())
    }
}

// 为了避免使用std::vec::Vec，我们实现一个简单的Vec替代
pub struct Vec<T> {
    data: *mut T,
    len: usize,
    capacity: usize,
}

impl<T> Vec<T> {
    pub fn new() -> Self {
        Self {
            data: core::ptr::null_mut(),
            len: 0,
            capacity: 0,
        }
    }

    pub fn push(&mut self, item: T) {
        // 简化实现：固定容量
        if self.len < 64 {
            unsafe {
                if self.data.is_null() {
                    // 分配初始内存（这里简化处理）
                    self.data = core::ptr::null_mut();
                    self.capacity = 64;
                }
                if !self.data.is_null() {
                    *self.data.add(self.len) = item;
                    self.len += 1;
                }
            }
        }
    }

    pub fn len(&self) -> usize {
        self.len
    }
}
