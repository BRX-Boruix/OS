//! Boruix OS 内存管理系统 - Rust实现
//! x86_64架构的完整内存管理，包括页表管理、物理内存分配和内核堆

#![no_std]
#![no_main]

use core::panic::PanicInfo;

pub mod arch;
pub mod ffi;
pub mod hhdm;  // HHDM支持
pub mod lazy_buddy;  // 懒加载伙伴分配器
pub mod paging;  // 分页管理
pub mod vmm;  // 虚拟内存管理
pub mod stats;

// 导出主要接口
pub use stats::*;
pub use paging::*;
pub use vmm::*;

/// 全局内存管理器实例
static mut MEMORY_MANAGER: Option<MemoryManager> = None;

/// 内存管理器主结构
/// 阶段2D: 添加虚拟内存管理
pub struct MemoryManager {
    physical_allocator: lazy_buddy::LazyBuddyAllocator,
    page_table_manager: Option<paging::PageTableManager>,
    vmm: Option<vmm::VirtualMemoryManager>,
    stats: stats::MemoryStats,
}

impl MemoryManager {
    /// 创建新的内存管理器
    /// 阶段2D: 添加虚拟内存管理
    pub const fn new() -> Self {
        Self {
            physical_allocator: lazy_buddy::LazyBuddyAllocator::new(),
            page_table_manager: None,
            vmm: None,
            stats: stats::MemoryStats::new(),
        }
    }

    /// 初始化内存管理器
    /// 阶段2D: 初始化物理分配器、页表管理器和VMM
    pub fn init(&mut self, memory_map: &[arch::MemoryRegion]) -> Result<(), &'static str> {
        // 初始化物理内存分配器
        self.physical_allocator.init(memory_map)?;

        // 初始化页表管理器(使用当前CR3)
        self.page_table_manager = Some(paging::PageTableManager::from_current()?);

        // 初始化虚拟内存管理器
        let mut vmm = vmm::VirtualMemoryManager::new();
        vmm.init()?;
        self.vmm = Some(vmm);

        // 初始化统计信息
        self.stats.init(memory_map);

        Ok(())
    }

    /// 获取全局内存管理器实例
    pub fn instance() -> &'static mut Self {
        unsafe {
            MEMORY_MANAGER.as_mut().unwrap_or_else(|| {
                panic!("Memory manager not initialized");
            })
        }
    }

    /// 设置全局内存管理器实例
    pub fn set_instance(manager: MemoryManager) {
        unsafe {
            MEMORY_MANAGER = Some(manager);
        }
    }
}

/// Panic处理函数
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}
