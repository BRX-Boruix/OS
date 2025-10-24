// paging.rs - x86_64 4级页表管理
// 支持页表创建、映射、取消映射和地址转换

use crate::hhdm;
use crate::arch::addr::{PhysAddr, VirtAddr};
use crate::lazy_buddy::PhysFrame;

// 页表项标志位
pub const PAGE_PRESENT: u64 = 1 << 0;      // 页面存在
pub const PAGE_WRITABLE: u64 = 1 << 1;     // 可写
pub const PAGE_USER: u64 = 1 << 2;         // 用户可访问
pub const PAGE_WRITE_THROUGH: u64 = 1 << 3; // 写穿透
pub const PAGE_CACHE_DISABLE: u64 = 1 << 4; // 禁用缓存
pub const PAGE_ACCESSED: u64 = 1 << 5;     // 已访问
pub const PAGE_DIRTY: u64 = 1 << 6;        // 已修改(仅1GB/2MB/4KB页)
pub const PAGE_HUGE: u64 = 1 << 7;         // 大页(2MB/1GB)
pub const PAGE_GLOBAL: u64 = 1 << 8;       // 全局页
pub const PAGE_NO_EXECUTE: u64 = 1 << 63;  // 不可执行(需要NXE支持)

// 页表索引相关常量
const PAGE_SIZE: u64 = 4096;
const ENTRIES_PER_TABLE: usize = 512;
const ENTRY_MASK: u64 = 0x000F_FFFF_FFFF_F000; // 物理地址掩码

// 页表结构 (每个页表512个条目，每个条目8字节)
#[repr(C, align(4096))]
pub struct PageTable {
    entries: [PageTableEntry; ENTRIES_PER_TABLE],
}

// 页表项
#[derive(Clone, Copy)]
#[repr(transparent)]
pub struct PageTableEntry {
    entry: u64,
}

impl PageTableEntry {
    // 创建未使用的页表项
    pub const fn unused() -> Self {
        PageTableEntry { entry: 0 }
    }

    // 检查页表项是否存在
    pub fn is_present(&self) -> bool {
        self.entry & PAGE_PRESENT != 0
    }

    // 检查是否可写
    pub fn is_writable(&self) -> bool {
        self.entry & PAGE_WRITABLE != 0
    }

    // 检查是否是大页
    pub fn is_huge(&self) -> bool {
        self.entry & PAGE_HUGE != 0
    }

    // 获取物理地址
    pub fn phys_addr(&self) -> Option<PhysAddr> {
        if self.is_present() {
            Some(PhysAddr::new(self.entry & ENTRY_MASK))
        } else {
            None
        }
    }

    // 获取标志位
    pub fn flags(&self) -> u64 {
        self.entry & !ENTRY_MASK
    }

    // 设置页表项
    pub fn set(&mut self, addr: PhysAddr, flags: u64) {
        self.entry = (addr.as_u64() & ENTRY_MASK) | flags;
    }

    // 清除页表项
    pub fn clear(&mut self) {
        self.entry = 0;
    }
}

impl PageTable {
    // 创建清空的页表
    pub fn zero() -> Self {
        PageTable {
            entries: [PageTableEntry::unused(); ENTRIES_PER_TABLE],
        }
    }

    // 获取页表项
    pub fn get_entry(&self, index: usize) -> Option<&PageTableEntry> {
        if index < ENTRIES_PER_TABLE {
            Some(&self.entries[index])
        } else {
            None
        }
    }

    // 获取可变页表项
    pub fn get_entry_mut(&mut self, index: usize) -> Option<&mut PageTableEntry> {
        if index < ENTRIES_PER_TABLE {
            Some(&mut self.entries[index])
        } else {
            None
        }
    }

    // 清空页表
    pub fn zero_entries(&mut self) {
        for entry in self.entries.iter_mut() {
            entry.clear();
        }
    }
}

// 页表管理器
pub struct PageTableManager {
    // CR3寄存器值(PML4物理地址)
    pml4_addr: PhysAddr,
}

impl PageTableManager {
    // 从当前CR3创建管理器
    pub fn from_current() -> Result<Self, &'static str> {
        let cr3: u64;
        unsafe {
            core::arch::asm!("mov {}, cr3", out(reg) cr3);
        }
        let pml4_addr = PhysAddr::new(cr3 & ENTRY_MASK);
        Ok(PageTableManager { pml4_addr })
    }

    // 创建新的页表(分配新的PML4)
    pub fn new<F>(alloc_frame: F) -> Result<Self, &'static str>
    where
        F: Fn() -> Option<PhysFrame>,
    {
        // 分配PML4页面
        let pml4_frame = alloc_frame().ok_or("Failed to allocate PML4 frame")?;
        let pml4_addr = pml4_frame.addr();

        // 通过HHDM访问并清空PML4
        let pml4_virt = hhdm::phys_to_virt(pml4_addr);
        let pml4_ptr = pml4_virt.as_u64() as *mut PageTable;
        unsafe {
            (*pml4_ptr).zero_entries();
        }

        Ok(PageTableManager { pml4_addr })
    }

    // 获取PML4物理地址
    pub fn pml4_addr(&self) -> PhysAddr {
        self.pml4_addr
    }

    // 加载此页表到CR3
    pub unsafe fn load(&self) {
        core::arch::asm!("mov cr3, {}", in(reg) self.pml4_addr.as_u64());
    }

    // 获取虚拟地址的页表索引
    fn get_page_table_indices(virt: VirtAddr) -> [usize; 4] {
        let addr = virt.as_u64();
        [
            ((addr >> 39) & 0x1FF) as usize, // PML4索引
            ((addr >> 30) & 0x1FF) as usize, // PDPT索引
            ((addr >> 21) & 0x1FF) as usize, // PD索引
            ((addr >> 12) & 0x1FF) as usize, // PT索引
        ]
    }

    // 获取或创建下一级页表
    fn get_or_create_next_table<F>(
        entry: &mut PageTableEntry,
        alloc_frame: &mut F,
    ) -> Result<*mut PageTable, &'static str>
    where
        F: FnMut() -> Option<PhysFrame>,
    {
        if entry.is_present() {
            // 页表已存在
            let phys = entry.phys_addr().ok_or("Invalid page table entry")?;
            let virt = hhdm::phys_to_virt(phys);
            Ok(virt.as_u64() as *mut PageTable)
        } else {
            // 需要创建新页表
            let frame = alloc_frame().ok_or("Failed to allocate page table frame")?;
            let phys = frame.addr();
            let virt = hhdm::phys_to_virt(phys);
            let table_ptr = virt.as_u64() as *mut PageTable;

            // 清空新页表
            unsafe {
                (*table_ptr).zero_entries();
            }

            // 设置页表项(存在、可写)
            entry.set(phys, PAGE_PRESENT | PAGE_WRITABLE);

            Ok(table_ptr)
        }
    }

    // 映射页面
    pub fn map_page<F>(
        &mut self,
        virt: VirtAddr,
        phys: PhysAddr,
        flags: u64,
        mut alloc_frame: F,
    ) -> Result<(), &'static str>
    where
        F: FnMut() -> Option<PhysFrame>,
    {
        // 检查地址对齐
        if virt.as_u64() % PAGE_SIZE != 0 || phys.as_u64() % PAGE_SIZE != 0 {
            return Err("Address not page aligned");
        }

        // 获取页表索引
        let indices = Self::get_page_table_indices(virt);

        // 遍历页表层级
        let pml4_virt = hhdm::phys_to_virt(self.pml4_addr);
        let mut current_table = pml4_virt.as_u64() as *mut PageTable;

        // PML4 -> PDPT
        let pml4_entry = unsafe { (*current_table).get_entry_mut(indices[0]).unwrap() };
        current_table = Self::get_or_create_next_table(pml4_entry, &mut alloc_frame)?;

        // PDPT -> PD
        let pdpt_entry = unsafe { (*current_table).get_entry_mut(indices[1]).unwrap() };
        current_table = Self::get_or_create_next_table(pdpt_entry, &mut alloc_frame)?;

        // PD -> PT
        let pd_entry = unsafe { (*current_table).get_entry_mut(indices[2]).unwrap() };
        current_table = Self::get_or_create_next_table(pd_entry, &mut alloc_frame)?;

        // PT -> Page
        let pt_entry = unsafe { (*current_table).get_entry_mut(indices[3]).unwrap() };
        if pt_entry.is_present() {
            return Err("Page already mapped");
        }

        // 设置最终页面映射
        pt_entry.set(phys, flags | PAGE_PRESENT);

        // 刷新TLB
        unsafe {
            core::arch::asm!("invlpg [{}]", in(reg) virt.as_u64());
        }

        Ok(())
    }

    // 取消映射页面
    pub fn unmap_page(&mut self, virt: VirtAddr) -> Result<PhysAddr, &'static str> {
        // 检查地址对齐
        if virt.as_u64() % PAGE_SIZE != 0 {
            return Err("Address not page aligned");
        }

        // 获取页表索引
        let indices = Self::get_page_table_indices(virt);

        // 遍历页表层级
        let pml4_virt = hhdm::phys_to_virt(self.pml4_addr);
        let pml4 = unsafe { &*(pml4_virt.as_u64() as *const PageTable) };

        // PML4 -> PDPT
        let pml4_entry = pml4.get_entry(indices[0]).unwrap();
        if !pml4_entry.is_present() {
            return Err("PDPT not present");
        }
        let pdpt_virt = hhdm::phys_to_virt(pml4_entry.phys_addr().unwrap());
        let pdpt = unsafe { &*(pdpt_virt.as_u64() as *const PageTable) };

        // PDPT -> PD
        let pdpt_entry = pdpt.get_entry(indices[1]).unwrap();
        if !pdpt_entry.is_present() {
            return Err("PD not present");
        }
        let pd_virt = hhdm::phys_to_virt(pdpt_entry.phys_addr().unwrap());
        let pd = unsafe { &*(pd_virt.as_u64() as *const PageTable) };

        // PD -> PT
        let pd_entry = pd.get_entry(indices[2]).unwrap();
        if !pd_entry.is_present() {
            return Err("PT not present");
        }
        let pt_virt = hhdm::phys_to_virt(pd_entry.phys_addr().unwrap());
        let pt = unsafe { &mut *(pt_virt.as_u64() as *mut PageTable) };

        // PT -> Page
        let pt_entry = pt.get_entry_mut(indices[3]).unwrap();
        if !pt_entry.is_present() {
            return Err("Page not mapped");
        }

        let phys = pt_entry.phys_addr().unwrap();
        pt_entry.clear();

        // 刷新TLB
        unsafe {
            core::arch::asm!("invlpg [{}]", in(reg) virt.as_u64());
        }

        Ok(phys)
    }

    // 获取物理地址(虚拟地址转换)
    pub fn translate(&self, virt: VirtAddr) -> Result<PhysAddr, &'static str> {
        // 获取页表索引
        let indices = Self::get_page_table_indices(virt);
        let offset = virt.as_u64() & 0xFFF; // 页内偏移

        // 遍历页表层级
        let pml4_virt = hhdm::phys_to_virt(self.pml4_addr);
        let pml4 = unsafe { &*(pml4_virt.as_u64() as *const PageTable) };

        // PML4 -> PDPT
        let pml4_entry = pml4.get_entry(indices[0]).unwrap();
        if !pml4_entry.is_present() {
            return Err("PDPT not present");
        }
        let pdpt_virt = hhdm::phys_to_virt(pml4_entry.phys_addr().unwrap());
        let pdpt = unsafe { &*(pdpt_virt.as_u64() as *const PageTable) };

        // PDPT -> PD
        let pdpt_entry = pdpt.get_entry(indices[1]).unwrap();
        if !pdpt_entry.is_present() {
            return Err("PD not present");
        }
        let pd_virt = hhdm::phys_to_virt(pdpt_entry.phys_addr().unwrap());
        let pd = unsafe { &*(pd_virt.as_u64() as *const PageTable) };

        // PD -> PT
        let pd_entry = pd.get_entry(indices[2]).unwrap();
        if !pd_entry.is_present() {
            return Err("PT not present");
        }
        let pt_virt = hhdm::phys_to_virt(pd_entry.phys_addr().unwrap());
        let pt = unsafe { &*(pt_virt.as_u64() as *const PageTable) };

        // PT -> Page
        let pt_entry = pt.get_entry(indices[3]).unwrap();
        if !pt_entry.is_present() {
            return Err("Page not mapped");
        }

        let page_phys = pt_entry.phys_addr().unwrap();
        Ok(PhysAddr::new(page_phys.as_u64() + offset))
    }
}

