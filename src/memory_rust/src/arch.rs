//! x86_64架构相关定义和常量

use core::fmt;

/// 页面大小常量
pub const PAGE_SIZE: usize = 4096;
pub const PAGE_SHIFT: usize = 12;
pub const PAGE_MASK: u64 = !(PAGE_SIZE as u64 - 1);

/// 页表项标志位
pub const PAGE_PRESENT: u64 = 1 << 0;
pub const PAGE_WRITABLE: u64 = 1 << 1;
pub const PAGE_USER: u64 = 1 << 2;
pub const PAGE_WRITE_THROUGH: u64 = 1 << 3;
pub const PAGE_CACHE_DISABLE: u64 = 1 << 4;
pub const PAGE_ACCESSED: u64 = 1 << 5;
pub const PAGE_DIRTY: u64 = 1 << 6;
pub const PAGE_SIZE_FLAG: u64 = 1 << 7; // 2MB页面
pub const PAGE_GLOBAL: u64 = 1 << 8;
pub const PAGE_NO_EXECUTE: u64 = 1 << 63;

/// 页表索引宏
pub const fn pml4_index(addr: u64) -> usize {
    ((addr >> 39) & 0x1FF) as usize
}

pub const fn pdp_index(addr: u64) -> usize {
    ((addr >> 30) & 0x1FF) as usize
}

pub const fn pd_index(addr: u64) -> usize {
    ((addr >> 21) & 0x1FF) as usize
}

pub const fn pt_index(addr: u64) -> usize {
    ((addr >> 12) & 0x1FF) as usize
}

/// 虚拟地址空间布局
pub const KERNEL_VIRTUAL_BASE: u64 = 0xFFFFFFFF80000000;
pub const USER_VIRTUAL_BASE: u64 = 0x0000000000400000;
pub const USER_VIRTUAL_END: u64 = 0x00007FFFFFFFFFFF;
pub const KERNEL_HEAP_START: u64 = 0xFFFFFFFF90000000;
pub const KERNEL_HEAP_END: u64 = 0xFFFFFFFFA0000000;

/// 页表项结构
#[repr(transparent)]
#[derive(Clone, Copy, PartialEq, Eq)]
pub struct PageTableEntry(pub u64);

impl PageTableEntry {
    /// 创建新的页表项
    pub const fn new() -> Self {
        Self(0)
    }

    /// 创建带有地址和标志的页表项
    pub const fn new_with_addr(addr: u64, flags: u64) -> Self {
        Self((addr & PAGE_MASK) | flags)
    }

    /// 检查是否存在
    pub const fn is_present(&self) -> bool {
        (self.0 & PAGE_PRESENT) != 0
    }

    /// 检查是否可写
    pub const fn is_writable(&self) -> bool {
        (self.0 & PAGE_WRITABLE) != 0
    }

    /// 检查是否用户可访问
    pub const fn is_user(&self) -> bool {
        (self.0 & PAGE_USER) != 0
    }

    /// 获取物理地址
    pub const fn addr(&self) -> u64 {
        self.0 & PAGE_MASK
    }

    /// 获取标志位
    pub const fn flags(&self) -> u64 {
        self.0 & !PAGE_MASK
    }

    /// 设置地址
    pub fn set_addr(&mut self, addr: u64) {
        self.0 = (self.0 & !PAGE_MASK) | (addr & PAGE_MASK);
    }

    /// 设置标志位
    pub fn set_flags(&mut self, flags: u64) {
        self.0 = (self.0 & PAGE_MASK) | (flags & !PAGE_MASK);
    }

    /// 设置存在标志
    pub fn set_present(&mut self, present: bool) {
        if present {
            self.0 |= PAGE_PRESENT;
        } else {
            self.0 &= !PAGE_PRESENT;
        }
    }

    /// 设置可写标志
    pub fn set_writable(&mut self, writable: bool) {
        if writable {
            self.0 |= PAGE_WRITABLE;
        } else {
            self.0 &= !PAGE_WRITABLE;
        }
    }

    /// 设置用户标志
    pub fn set_user(&mut self, user: bool) {
        if user {
            self.0 |= PAGE_USER;
        } else {
            self.0 &= !PAGE_USER;
        }
    }

    /// 清零页表项
    pub fn clear(&mut self) {
        self.0 = 0;
    }
}

impl fmt::Debug for PageTableEntry {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("PageTableEntry")
            .field("addr", &format_args!("0x{:016x}", self.addr()))
            .field("present", &self.is_present())
            .field("writable", &self.is_writable())
            .field("user", &self.is_user())
            .field("flags", &format_args!("0x{:x}", self.flags()))
            .finish()
    }
}

/// 页表结构
#[repr(align(4096))]
pub struct PageTable {
    pub entries: [PageTableEntry; 512],
}

impl PageTable {
    /// 创建新的页表
    pub const fn new() -> Self {
        Self {
            entries: [PageTableEntry::new(); 512],
        }
    }

    /// 清空页表
    pub fn clear(&mut self) {
        for entry in &mut self.entries {
            entry.clear();
        }
    }

    /// 获取页表项
    pub fn get_entry(&self, index: usize) -> Option<&PageTableEntry> {
        self.entries.get(index)
    }

    /// 获取可变页表项
    pub fn get_entry_mut(&mut self, index: usize) -> Option<&mut PageTableEntry> {
        self.entries.get_mut(index)
    }
}

/// 内存区域类型
#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MemoryType {
    Available = 1,
    Reserved = 2,
    AcpiReclaimable = 3,
    AcpiNvs = 4,
    Bad = 5,
}

/// 内存区域描述符
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct MemoryRegion {
    pub base_addr: u64,
    pub length: u64,
    pub memory_type: MemoryType,
}

impl MemoryRegion {
    /// 创建新的内存区域
    pub const fn new(base_addr: u64, length: u64, memory_type: MemoryType) -> Self {
        Self {
            base_addr,
            length,
            memory_type,
        }
    }

    /// 获取结束地址
    pub const fn end_addr(&self) -> u64 {
        self.base_addr + self.length
    }

    /// 检查是否包含地址
    pub const fn contains(&self, addr: u64) -> bool {
        addr >= self.base_addr && addr < self.end_addr()
    }

    /// 检查是否可用
    pub const fn is_available(&self) -> bool {
        matches!(self.memory_type, MemoryType::Available)
    }
}

/// CPU相关操作
pub mod cpu {
    /// 刷新TLB
    pub unsafe fn flush_tlb(addr: u64) {
        core::arch::asm!("invlpg [{}]", in(reg) addr, options(nostack, preserves_flags));
    }

    /// 刷新所有TLB
    pub unsafe fn flush_all_tlb() {
        let cr3: u64;
        core::arch::asm!("mov {}, cr3", out(reg) cr3, options(nostack, preserves_flags));
        core::arch::asm!("mov cr3, {}", in(reg) cr3, options(nostack, preserves_flags));
    }

    /// 获取CR3寄存器值
    pub unsafe fn get_cr3() -> u64 {
        let cr3: u64;
        core::arch::asm!("mov {}, cr3", out(reg) cr3, options(nostack, preserves_flags));
        cr3
    }

    /// 设置CR3寄存器值
    pub unsafe fn set_cr3(cr3: u64) {
        core::arch::asm!("mov cr3, {}", in(reg) cr3, options(nostack, preserves_flags));
    }
}

/// 地址转换工具
pub mod addr {
    use super::PAGE_SIZE;

    /// 虚拟地址
    #[repr(transparent)]
    #[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
    pub struct VirtAddr(pub u64);

    impl VirtAddr {
        /// 创建新的虚拟地址
        pub const fn new(addr: u64) -> Self {
            Self(addr)
        }

        /// 获取地址值
        pub const fn as_u64(&self) -> u64 {
            self.0
        }

        /// 页面对齐
        pub const fn page_align_down(&self) -> Self {
            Self(self.0 & !(PAGE_SIZE as u64 - 1))
        }

        /// 页面对齐（向上）
        pub const fn page_align_up(&self) -> Self {
            Self((self.0 + PAGE_SIZE as u64 - 1) & !(PAGE_SIZE as u64 - 1))
        }

        /// 检查是否页面对齐
        pub const fn is_page_aligned(&self) -> bool {
            (self.0 & (PAGE_SIZE as u64 - 1)) == 0
        }

        /// 获取页面偏移
        pub const fn page_offset(&self) -> u64 {
            self.0 & (PAGE_SIZE as u64 - 1)
        }
    }

    /// 物理地址
    #[repr(transparent)]
    #[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
    pub struct PhysAddr(pub u64);

    impl PhysAddr {
        /// 创建新的物理地址
        pub const fn new(addr: u64) -> Self {
            Self(addr)
        }

        /// 获取地址值
        pub const fn as_u64(&self) -> u64 {
            self.0
        }

        /// 页面对齐
        pub const fn page_align_down(&self) -> Self {
            Self(self.0 & !(PAGE_SIZE as u64 - 1))
        }

        /// 页面对齐（向上）
        pub const fn page_align_up(&self) -> Self {
            Self((self.0 + PAGE_SIZE as u64 - 1) & !(PAGE_SIZE as u64 - 1))
        }

        /// 检查是否页面对齐
        pub const fn is_page_aligned(&self) -> bool {
            (self.0 & (PAGE_SIZE as u64 - 1)) == 0
        }
    }
}
