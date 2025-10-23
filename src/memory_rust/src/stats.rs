//! 内存统计和调试功能
//! 提供内存使用情况的统计和调试信息

use crate::arch::MemoryRegion;
use crate::heap::HeapStats;

/// 内存统计信息
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct MemoryStats {
    /// 物理内存统计
    pub physical: PhysicalMemoryStats,
    /// 虚拟内存统计
    pub virtual_memory: VirtualMemoryStats,
    /// 堆内存统计
    pub heap: HeapStats,
    /// 页表统计
    pub page_tables: PageTableStats,
}

/// 物理内存统计
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct PhysicalMemoryStats {
    /// 总物理内存大小（字节）
    pub total_memory: u64,
    /// 可用内存大小（字节）
    pub available_memory: u64,
    /// 已分配内存大小（字节）
    pub allocated_memory: u64,
    /// 保留内存大小（字节）
    pub reserved_memory: u64,
    /// 总页面数
    pub total_pages: usize,
    /// 已分配页面数
    pub allocated_pages: usize,
    /// 空闲页面数
    pub free_pages: usize,
    /// 内存使用率（百分比）
    pub usage_percent: u32,
}

/// 虚拟内存统计
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct VirtualMemoryStats {
    /// 内核虚拟地址空间大小
    pub kernel_virtual_size: u64,
    /// 用户虚拟地址空间大小
    pub user_virtual_size: u64,
    /// 已映射的虚拟页面数
    pub mapped_pages: usize,
    /// 页表占用的内存大小
    pub page_table_memory: usize,
}

/// 页表统计
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct PageTableStats {
    /// PML4表数量
    pub pml4_tables: usize,
    /// PDP表数量
    pub pdp_tables: usize,
    /// PD表数量
    pub pd_tables: usize,
    /// PT表数量
    pub pt_tables: usize,
    /// 页表项总数
    pub total_entries: usize,
    /// 有效页表项数量
    pub valid_entries: usize,
}

impl MemoryStats {
    /// 创建新的内存统计
    pub const fn new() -> Self {
        Self {
            physical: PhysicalMemoryStats::new(),
            virtual_memory: VirtualMemoryStats::new(),
            heap: HeapStats {
                total_size: 0,
                allocated: 0,
                free: 0,
                peak_usage: 0,
                total_allocated: 0,
                total_freed: 0,
            },
            page_tables: PageTableStats::new(),
        }
    }

    /// 初始化内存统计
    pub fn init(&mut self, memory_regions: &[MemoryRegion]) {
        self.physical.init(memory_regions);
        self.virtual_memory.init();
        self.page_tables.init();
    }

    /// 更新统计信息
    pub fn update(
        &mut self,
        allocated_pages: usize,
        heap_stats: HeapStats,
        page_table_stats: PageTableStats,
    ) {
        self.physical.update_allocation(allocated_pages);
        self.heap = heap_stats;
        self.page_tables = page_table_stats;
    }

    /// 获取内存使用摘要
    pub fn summary(&self) -> MemorySummary {
        MemorySummary {
            total_physical_mb: self.physical.total_memory / (1024 * 1024),
            used_physical_mb: self.physical.allocated_memory / (1024 * 1024),
            free_physical_mb: (self.physical.total_memory - self.physical.allocated_memory)
                / (1024 * 1024),
            heap_used_kb: self.heap.allocated / 1024,
            heap_free_kb: self.heap.free / 1024,
            page_tables_count: self.page_tables.pml4_tables
                + self.page_tables.pdp_tables
                + self.page_tables.pd_tables
                + self.page_tables.pt_tables,
            usage_percent: self.physical.usage_percent,
        }
    }
}

impl PhysicalMemoryStats {
    /// 创建新的物理内存统计
    pub const fn new() -> Self {
        Self {
            total_memory: 0,
            available_memory: 0,
            allocated_memory: 0,
            reserved_memory: 0,
            total_pages: 0,
            allocated_pages: 0,
            free_pages: 0,
            usage_percent: 0,
        }
    }

    /// 初始化物理内存统计
    pub fn init(&mut self, memory_regions: &[MemoryRegion]) {
        self.total_memory = 0;
        self.available_memory = 0;
        self.reserved_memory = 0;

        for region in memory_regions {
            self.total_memory += region.length;

            match region.memory_type {
                crate::arch::MemoryType::Available => {
                    self.available_memory += region.length;
                }
                _ => {
                    self.reserved_memory += region.length;
                }
            }
        }

        self.total_pages = (self.available_memory / 4096) as usize;
        self.free_pages = self.total_pages;
        self.allocated_pages = 0;
        self.allocated_memory = 0;
        self.usage_percent = 0;
    }

    /// 更新分配统计
    pub fn update_allocation(&mut self, allocated_pages: usize) {
        self.allocated_pages = allocated_pages;
        self.free_pages = self.total_pages.saturating_sub(allocated_pages);
        self.allocated_memory = (allocated_pages * 4096) as u64;

        if self.total_pages > 0 {
            self.usage_percent = (allocated_pages * 100 / self.total_pages) as u32;
        }
    }
}

impl VirtualMemoryStats {
    /// 创建新的虚拟内存统计
    pub const fn new() -> Self {
        Self {
            kernel_virtual_size: 0,
            user_virtual_size: 0,
            mapped_pages: 0,
            page_table_memory: 0,
        }
    }

    /// 初始化虚拟内存统计
    pub fn init(&mut self) {
        use crate::arch::{
            KERNEL_HEAP_END, KERNEL_VIRTUAL_BASE, USER_VIRTUAL_BASE, USER_VIRTUAL_END,
        };

        self.kernel_virtual_size = KERNEL_HEAP_END - KERNEL_VIRTUAL_BASE;
        self.user_virtual_size = USER_VIRTUAL_END - USER_VIRTUAL_BASE;
        self.mapped_pages = 0;
        self.page_table_memory = 0;
    }
}

impl PageTableStats {
    /// 创建新的页表统计
    pub const fn new() -> Self {
        Self {
            pml4_tables: 0,
            pdp_tables: 0,
            pd_tables: 0,
            pt_tables: 0,
            total_entries: 0,
            valid_entries: 0,
        }
    }

    /// 初始化页表统计
    pub fn init(&mut self) {
        self.pml4_tables = 1; // 至少有一个PML4表
        self.pdp_tables = 0;
        self.pd_tables = 0;
        self.pt_tables = 0;
        self.total_entries = 512; // PML4表的512个条目
        self.valid_entries = 0;
    }

    /// 更新页表统计
    pub fn update(&mut self, pml4: usize, pdp: usize, pd: usize, pt: usize, valid: usize) {
        self.pml4_tables = pml4;
        self.pdp_tables = pdp;
        self.pd_tables = pd;
        self.pt_tables = pt;
        self.total_entries = (pml4 + pdp + pd + pt) * 512;
        self.valid_entries = valid;
    }
}

/// 内存使用摘要
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct MemorySummary {
    /// 总物理内存（MB）
    pub total_physical_mb: u64,
    /// 已使用物理内存（MB）
    pub used_physical_mb: u64,
    /// 空闲物理内存（MB）
    pub free_physical_mb: u64,
    /// 堆已使用内存（KB）
    pub heap_used_kb: usize,
    /// 堆空闲内存（KB）
    pub heap_free_kb: usize,
    /// 页表总数
    pub page_tables_count: usize,
    /// 内存使用率（百分比）
    pub usage_percent: u32,
}

/// 内存调试信息
pub struct MemoryDebugInfo {
    /// 内存区域信息
    pub regions: [Option<MemoryRegion>; 32],
    /// 区域数量
    pub region_count: usize,
    /// 最近的分配记录
    pub recent_allocations: [AllocationRecord; 16],
    /// 分配记录数量
    pub allocation_count: usize,
}

/// 分配记录
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct AllocationRecord {
    /// 分配地址
    pub address: u64,
    /// 分配大小
    pub size: usize,
    /// 分配时间戳（简化为计数器）
    pub timestamp: u64,
    /// 是否已释放
    pub is_freed: bool,
}

impl MemoryDebugInfo {
    /// 创建新的调试信息
    pub const fn new() -> Self {
        Self {
            regions: [None; 32],
            region_count: 0,
            recent_allocations: [AllocationRecord::new(); 16],
            allocation_count: 0,
        }
    }

    /// 添加内存区域
    pub fn add_region(&mut self, region: MemoryRegion) {
        if self.region_count < self.regions.len() {
            self.regions[self.region_count] = Some(region);
            self.region_count += 1;
        }
    }

    /// 记录分配
    pub fn record_allocation(&mut self, address: u64, size: usize) {
        let index = self.allocation_count % self.recent_allocations.len();
        self.recent_allocations[index] = AllocationRecord {
            address,
            size,
            timestamp: self.allocation_count as u64,
            is_freed: false,
        };
        self.allocation_count += 1;
    }

    /// 记录释放
    pub fn record_deallocation(&mut self, address: u64) {
        for record in &mut self.recent_allocations {
            if record.address == address && !record.is_freed {
                record.is_freed = true;
                break;
            }
        }
    }
}

impl AllocationRecord {
    /// 创建新的分配记录
    pub const fn new() -> Self {
        Self {
            address: 0,
            size: 0,
            timestamp: 0,
            is_freed: false,
        }
    }
}

/// 内存诊断工具
pub struct MemoryDiagnostics;

impl MemoryDiagnostics {
    /// 检查内存泄漏
    pub fn check_memory_leaks(debug_info: &MemoryDebugInfo) -> usize {
        let mut leak_count = 0;

        for record in &debug_info.recent_allocations {
            if record.address != 0 && !record.is_freed {
                leak_count += 1;
            }
        }

        leak_count
    }

    /// 检查内存碎片化程度
    pub fn check_fragmentation(stats: &MemoryStats) -> u32 {
        // 简化的碎片化检查：基于堆的使用情况
        if stats.heap.total_size == 0 {
            return 0;
        }

        let fragmentation = (stats.heap.free * 100) / stats.heap.total_size;
        if fragmentation > 50 {
            fragmentation as u32
        } else {
            0
        }
    }

    /// 生成内存报告
    pub fn generate_report(stats: &MemoryStats, debug_info: &MemoryDebugInfo) -> MemoryReport {
        let summary = stats.summary();
        let leaks = Self::check_memory_leaks(debug_info);
        let fragmentation = Self::check_fragmentation(stats);

        MemoryReport {
            summary,
            leak_count: leaks,
            fragmentation_percent: fragmentation,
            region_count: debug_info.region_count,
            allocation_count: debug_info.allocation_count,
        }
    }
}

/// 内存报告
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct MemoryReport {
    /// 内存摘要
    pub summary: MemorySummary,
    /// 内存泄漏数量
    pub leak_count: usize,
    /// 碎片化程度（百分比）
    pub fragmentation_percent: u32,
    /// 内存区域数量
    pub region_count: usize,
    /// 总分配次数
    pub allocation_count: usize,
}
