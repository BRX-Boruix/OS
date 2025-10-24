// protection.rs - 内存保护机制
// 页面权限管理和保护策略

use crate::arch::addr::VirtAddr;
use crate::paging::{PageTableManager, PAGE_PRESENT, PAGE_WRITABLE, PAGE_USER, PAGE_NO_EXECUTE};

/// 页面保护标志
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ProtectionFlags {
    pub present: bool,      // 页面存在
    pub writable: bool,     // 可写
    pub user: bool,         // 用户可访问
    pub executable: bool,   // 可执行
}

impl ProtectionFlags {
    /// 创建新的保护标志
    pub const fn new() -> Self {
        Self {
            present: true,
            writable: false,
            user: false,
            executable: false,
        }
    }

    /// 只读内核页面
    pub const fn kernel_ro() -> Self {
        Self {
            present: true,
            writable: false,
            user: false,
            executable: false,
        }
    }

    /// 可读写内核页面
    pub const fn kernel_rw() -> Self {
        Self {
            present: true,
            writable: true,
            user: false,
            executable: false,
        }
    }

    /// 可执行内核页面
    pub const fn kernel_rx() -> Self {
        Self {
            present: true,
            writable: false,
            user: false,
            executable: true,
        }
    }

    /// 只读用户页面
    pub const fn user_ro() -> Self {
        Self {
            present: true,
            writable: false,
            user: true,
            executable: false,
        }
    }

    /// 可读写用户页面
    pub const fn user_rw() -> Self {
        Self {
            present: true,
            writable: true,
            user: true,
            executable: false,
        }
    }

    /// 可执行用户页面
    pub const fn user_rx() -> Self {
        Self {
            present: true,
            writable: false,
            user: true,
            executable: true,
        }
    }

    /// 转换为页表标志位
    pub fn to_page_flags(&self) -> u64 {
        let mut flags = 0u64;
        
        if self.present {
            flags |= PAGE_PRESENT;
        }
        if self.writable {
            flags |= PAGE_WRITABLE;
        }
        if self.user {
            flags |= PAGE_USER;
        }
        if !self.executable {
            flags |= PAGE_NO_EXECUTE;
        }
        
        flags
    }

    /// 从页表标志位创建
    pub fn from_page_flags(flags: u64) -> Self {
        Self {
            present: flags & PAGE_PRESENT != 0,
            writable: flags & PAGE_WRITABLE != 0,
            user: flags & PAGE_USER != 0,
            executable: flags & PAGE_NO_EXECUTE == 0,
        }
    }
}

/// 内存保护管理器
pub struct ProtectionManager;

impl ProtectionManager {
    /// 设置页面保护
    pub fn set_page_protection(
        page_table: &mut PageTableManager,
        virt_addr: VirtAddr,
        protection: ProtectionFlags,
    ) -> Result<(), &'static str> {
        // 获取当前页面的物理地址
        let phys_addr = page_table.translate(virt_addr)?;
        
        // 取消映射
        page_table.unmap_page(virt_addr)?;
        
        // 使用新的保护标志重新映射
        let flags = protection.to_page_flags();
        page_table.map_page(virt_addr, phys_addr, flags, || None)?;
        
        Ok(())
    }

    /// 设置页面为只读
    pub fn set_readonly(
        page_table: &mut PageTableManager,
        virt_addr: VirtAddr,
    ) -> Result<(), &'static str> {
        Self::set_page_protection(page_table, virt_addr, ProtectionFlags::kernel_ro())
    }

    /// 设置页面为可读写
    pub fn set_readwrite(
        page_table: &mut PageTableManager,
        virt_addr: VirtAddr,
    ) -> Result<(), &'static str> {
        Self::set_page_protection(page_table, virt_addr, ProtectionFlags::kernel_rw())
    }

    /// 设置页面为不可执行
    pub fn set_no_execute(
        page_table: &mut PageTableManager,
        virt_addr: VirtAddr,
    ) -> Result<(), &'static str> {
        // 获取当前保护标志
        let phys_addr = page_table.translate(virt_addr)?;
        
        // 创建新的保护标志（保持其他位，只设置NX）
        let mut protection = ProtectionFlags::kernel_rw();
        protection.executable = false;
        
        // 取消映射并重新映射
        page_table.unmap_page(virt_addr)?;
        let flags = protection.to_page_flags();
        page_table.map_page(virt_addr, phys_addr, flags, || None)?;
        
        Ok(())
    }

    /// 获取页面保护标志
    pub fn get_page_protection(
        page_table: &PageTableManager,
        virt_addr: VirtAddr,
    ) -> Result<ProtectionFlags, &'static str> {
        // 获取页面标志位
        let flags = page_table.get_page_flags(virt_addr)?;
        
        // 从标志位创建保护标志
        Ok(ProtectionFlags::from_page_flags(flags))
    }

    /// 检查地址是否可写
    pub fn is_writable(
        page_table: &PageTableManager,
        virt_addr: VirtAddr,
    ) -> Result<bool, &'static str> {
        let protection = Self::get_page_protection(page_table, virt_addr)?;
        Ok(protection.writable)
    }

    /// 检查地址是否可执行
    pub fn is_executable(
        page_table: &PageTableManager,
        virt_addr: VirtAddr,
    ) -> Result<bool, &'static str> {
        let protection = Self::get_page_protection(page_table, virt_addr)?;
        Ok(protection.executable)
    }
}

/// 内存区域保护策略
#[derive(Debug, Clone, Copy)]
pub enum MemoryRegionType {
    KernelCode,      // 内核代码：RX (只读+可执行)
    KernelData,      // 内核数据：RW (可读写+不可执行)
    KernelRoData,    // 内核只读数据：R (只读+不可执行)
    KernelStack,     // 内核栈：RW (可读写+不可执行)
    UserCode,        // 用户代码：URX (用户+只读+可执行)
    UserData,        // 用户数据：URW (用户+可读写+不可执行)
    UserStack,       // 用户栈：URW (用户+可读写+不可执行)
}

impl MemoryRegionType {
    /// 获取区域的保护标志
    pub fn protection_flags(&self) -> ProtectionFlags {
        match self {
            MemoryRegionType::KernelCode => ProtectionFlags::kernel_rx(),
            MemoryRegionType::KernelData => ProtectionFlags::kernel_rw(),
            MemoryRegionType::KernelRoData => ProtectionFlags::kernel_ro(),
            MemoryRegionType::KernelStack => ProtectionFlags::kernel_rw(),
            MemoryRegionType::UserCode => ProtectionFlags::user_rx(),
            MemoryRegionType::UserData => ProtectionFlags::user_rw(),
            MemoryRegionType::UserStack => ProtectionFlags::user_rw(),
        }
    }
}

