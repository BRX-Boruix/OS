//! 内存统计和调试功能（精简版 - 阶段2）
//! 只包含物理内存统计

use crate::arch::MemoryRegion;

/// 物理内存统计
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct PhysicalStats {
    pub total_pages: usize,
    pub allocated_pages: usize,
    pub free_pages: usize,
}

impl PhysicalStats {
    pub const fn new() -> Self {
        Self {
            total_pages: 0,
            allocated_pages: 0,
            free_pages: 0,
        }
    }

    pub fn update_allocation(&mut self, allocated_pages: usize) {
        self.allocated_pages = allocated_pages;
        self.free_pages = self.total_pages.saturating_sub(allocated_pages);
    }
}

/// 内存统计信息（精简版）
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct MemoryStats {
    pub physical: PhysicalStats,
}

impl MemoryStats {
    pub const fn new() -> Self {
        Self {
            physical: PhysicalStats::new(),
        }
    }

    pub fn init(&mut self, memory_map: &[MemoryRegion]) {
        // 计算总物理页面数
        let mut total_pages = 0;
        for region in memory_map {
            if region.is_available() {
                total_pages += (region.length / 4096) as usize;
            }
        }
        self.physical.total_pages = total_pages;
    }

    pub fn update_physical_allocation(&mut self, allocated_pages: usize) {
        self.physical.update_allocation(allocated_pages);
    }
}

/// 内存摘要（C兼容）
#[repr(C)]
pub struct MemorySummary {
    pub total_physical_mb: u64,
    pub used_physical_mb: u64,
    pub free_physical_mb: u64,
}
