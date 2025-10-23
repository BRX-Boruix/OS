//! Boruix OS 内存管理系统 - Rust实现
//! x86_64架构的完整内存管理，包括页表管理、物理内存分配和内核堆

#![no_std]
#![no_main]

use core::panic::PanicInfo;

pub mod allocator;
pub mod arch;
pub mod ffi;
pub mod heap;
pub mod hhdm;  // HHDM支持
pub mod paging;
pub mod physical;
pub mod physical_simple;  // 阶段2使用简化版本
pub mod stats;

// 导出主要接口
pub use allocator::{
    Allocator, AllocatorError, AllocatorResult, PageAllocator, StatsProvider, VirtualAllocator,
};
pub use heap::{HeapStats, KernelHeap};
pub use paging::PageTableManager;
pub use physical::{PhysFrame, PhysicalAllocator};
pub use stats::*;

/// 全局内存管理器实例
static mut MEMORY_MANAGER: Option<MemoryManager> = None;

/// 内存管理器主结构
/// 阶段2: 使用简化的位图物理分配器（稳定版本）
pub struct MemoryManager {
    physical_allocator: physical_simple::PhysicalAllocator,
    page_table_manager: paging::PageTableManager,
    kernel_heap: heap::KernelHeap,
    stats: stats::MemoryStats,
}

impl MemoryManager {
    /// 创建新的内存管理器
    /// 阶段2: 使用简化的位图物理分配器
    pub const fn new() -> Self {
        Self {
            physical_allocator: physical_simple::PhysicalAllocator::new(),
            page_table_manager: paging::PageTableManager::new(),
            kernel_heap: heap::KernelHeap::new(),
            stats: stats::MemoryStats::new(),
        }
    }

    /// 初始化内存管理器
    /// 阶段2: 只初始化物理内存分配器
    pub fn init(&mut self, memory_map: &[arch::MemoryRegion]) -> Result<(), &'static str> {
        // 初始化物理内存分配器
        self.physical_allocator.init(memory_map)?;

        // 阶段2: 暂不初始化页表管理器
        // self.page_table_manager.init(&mut self.physical_allocator)?;

        // 阶段2: 暂不初始化内核堆
        // self.kernel_heap
        //     .init(&mut self.physical_allocator, &mut self.page_table_manager)?;

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
