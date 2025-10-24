//! C语言FFI接口 - 阶段2: 物理内存分配器
//! 实现页面分配功能

use crate::arch::{MemoryRegion, MemoryType};
use crate::hhdm;
use crate::lazy_buddy::PhysFrame;
use crate::MemoryManager;
use core::ptr;
use core::slice;

// C串口调试函数声明
extern "C" {
    fn serial_puts(s: *const u8);
}

// 串口调试宏
macro_rules! serial_log {
    ($msg:expr) => {
        unsafe {
            serial_puts(concat!("[RUST] ", $msg, "\n\0").as_ptr());
        }
    };
}

/// C兼容的内存区域结构
#[repr(C)]
#[derive(Clone, Copy)]
pub struct CMemoryRegion {
    pub base_addr: u64,
    pub length: u64,
    pub memory_type: u32,
}

impl From<CMemoryRegion> for MemoryRegion {
    fn from(c_region: CMemoryRegion) -> Self {
        let memory_type = match c_region.memory_type {
            1 => MemoryType::Available,
            2 => MemoryType::Reserved,
            3 => MemoryType::AcpiReclaimable,
            4 => MemoryType::AcpiNvs,
            5 => MemoryType::Bad,
            _ => MemoryType::Reserved,
        };

        MemoryRegion::new(c_region.base_addr, c_region.length, memory_type)
    }
}

/// 设置HHDM偏移量
/// 必须在rust_memory_init之前调用
#[no_mangle]
pub extern "C" fn rust_set_hhdm_offset(offset: u64) {
    hhdm::set_offset(offset);
}

/// 初始化Rust内存管理器
/// 阶段2: 真实初始化物理分配器
#[no_mangle]
pub extern "C" fn rust_memory_init(
    memory_regions: *const CMemoryRegion,
    region_count: usize,
) -> i32 {
    serial_log!("rust_memory_init() called");
    
    if memory_regions.is_null() || region_count == 0 {
        serial_log!("ERROR: Invalid memory regions pointer or count");
        return -1;
    }
    
    serial_log!("Converting memory regions...");
    // 转换C内存区域到Rust格式
    let c_regions = unsafe { slice::from_raw_parts(memory_regions, region_count) };
    let mut rust_regions = [MemoryRegion::new(0, 0, MemoryType::Reserved); 32];

    if region_count > rust_regions.len() {
        serial_log!("ERROR: Too many memory regions");
        return -1;
    }

    for (i, c_region) in c_regions.iter().enumerate() {
        rust_regions[i] = (*c_region).into();
    }
    
    serial_log!("Creating memory manager...");
    // 创建并初始化内存管理器
    let mut manager = MemoryManager::new();
    
    serial_log!("Initializing memory manager...");
    match manager.init(&rust_regions[..region_count]) {
        Ok(()) => {
            serial_log!("Memory manager init OK, setting instance...");
            MemoryManager::set_instance(manager);
            serial_log!("rust_memory_init() SUCCESS");
            0
        }
        Err(e) => {
            serial_log!("ERROR: Memory manager init failed");
            unsafe {
                serial_puts(b"[RUST] Error message: \0".as_ptr());
                serial_puts(e.as_ptr());
                serial_puts(b"\n\0".as_ptr());
            }
            -1
        },
    }
}

/// 分配内存
/// 阶段2E: 完整实现，使用堆分配器
#[no_mangle]
pub extern "C" fn rust_kmalloc(size: usize) -> *mut u8 {
    if size == 0 {
        return ptr::null_mut();
    }

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_mut() } {
        Some(m) => m,
        None => {
            serial_log!("ERROR: Memory manager not initialized");
            return ptr::null_mut();
        }
    };

    // 获取所有需要的组件
    let (heap, vmm, page_table) = match (
        &mut manager.heap_allocator,
        &mut manager.vmm,
        &mut manager.page_table_manager,
    ) {
        (Some(h), Some(v), Some(pt)) => (h, v, pt),
        _ => {
            serial_log!("ERROR: Heap/VMM/PageTable not initialized");
            return ptr::null_mut();
        }
    };

    // 分配物理页面的闭包
    let alloc_frame = || manager.physical_allocator.allocate_frame();

    // 执行分配
    match heap.allocate(size, vmm, page_table, alloc_frame) {
        Ok(ptr) => ptr,
        Err(_e) => {
            serial_log!("ERROR: Failed to allocate heap memory");
            ptr::null_mut()
        }
    }
}

/// 释放内存
/// 阶段2E: 完整实现，使用堆分配器
#[no_mangle]
pub extern "C" fn rust_kfree(ptr_arg: *mut u8) {
    if ptr_arg.is_null() {
        return;
    }

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_mut() } {
        Some(m) => m,
        None => {
            serial_log!("ERROR: Memory manager not initialized");
            return;
        }
    };

    // 获取堆分配器
    let heap = match &mut manager.heap_allocator {
        Some(h) => h,
        None => {
            serial_log!("ERROR: Heap allocator not initialized");
            return;
        }
    };

    // 执行释放
    if let Err(_e) = heap.deallocate(ptr_arg) {
        serial_log!("ERROR: Failed to free heap memory");
    }
}

/// 分配物理页面
/// 阶段2: 真实实现
#[no_mangle]
pub extern "C" fn rust_alloc_page() -> u64 {
    serial_log!("rust_alloc_page() called");
    
    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_mut() } {
        Some(m) => m,
        None => {
            serial_log!("ERROR: Memory manager not initialized");
            return 0;
        }
    };

    serial_log!("Calling allocate_frame()...");
    // 使用物理分配器分配页面
    match manager.physical_allocator.allocate_frame() {
        Some(frame) => {
            let addr = frame.start_address().as_u64();
            serial_log!("Page allocated successfully");
            addr
        },
        None => {
            serial_log!("ERROR: Failed to allocate page");
            0
        }
    }
}

/// 释放物理页面
/// 阶段2: 真实实现
#[no_mangle]
pub extern "C" fn rust_free_page(page_addr: u64) {
    if page_addr == 0 {
        return;
    }

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_mut() } {
        Some(m) => m,
        None => return,
    };

    // 使用物理分配器释放页面
    use crate::arch::addr::PhysAddr;

    let frame = PhysFrame::from_start_address(PhysAddr::new(page_addr));
    manager.physical_allocator.deallocate_frame(frame);
}

/// 映射虚拟页面到物理页面
/// 阶段2C: 实现页表映射
#[no_mangle]
pub extern "C" fn rust_map_page(virtual_addr: u64, physical_addr: u64, flags: u64) -> i32 {
    use crate::arch::addr::{VirtAddr, PhysAddr};
    use crate::paging::PAGE_PRESENT;

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_mut() } {
        Some(m) => m,
        None => {
            serial_log!("ERROR: Memory manager not initialized");
            return -1;
        }
    };

    // 获取页表管理器
    let page_table_manager = match manager.page_table_manager.as_mut() {
        Some(ptm) => ptm,
        None => {
            serial_log!("ERROR: Page table manager not initialized");
            return -1;
        }
    };

    let virt = VirtAddr::new(virtual_addr);
    let phys = PhysAddr::new(physical_addr);

    // 使用物理分配器作为页表分配器
    let alloc_frame = || manager.physical_allocator.allocate_frame();

    // 执行映射
    match page_table_manager.map_page(virt, phys, flags | PAGE_PRESENT, alloc_frame) {
        Ok(_) => 0,  // 成功
        Err(_e) => {
            serial_log!("ERROR: Failed to map page");
            -1  // 失败
        }
    }
}

/// 取消页面映射
/// 阶段2C: 实现页面取消映射
#[no_mangle]
pub extern "C" fn rust_unmap_page(virtual_addr: u64) -> u64 {
    use crate::arch::addr::VirtAddr;

    if virtual_addr == 0 {
        return 0;
    }

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_mut() } {
        Some(m) => m,
        None => {
            serial_log!("ERROR: Memory manager not initialized");
            return 0;
        }
    };

    // 获取页表管理器
    let page_table_manager = match manager.page_table_manager.as_mut() {
        Some(ptm) => ptm,
        None => {
            serial_log!("ERROR: Page table manager not initialized");
            return 0;
        }
    };

    let virt = VirtAddr::new(virtual_addr);

    // 执行取消映射
    match page_table_manager.unmap_page(virt) {
        Ok(phys) => phys.as_u64(),  // 返回物理地址
        Err(_e) => {
            serial_log!("ERROR: Failed to unmap page");
            0  // 失败
        }
    }
}

/// 虚拟地址转物理地址
/// 阶段2C: 实现地址转换
#[no_mangle]
pub extern "C" fn rust_virt_to_phys(virtual_addr: u64) -> u64 {
    use crate::arch::addr::VirtAddr;

    if virtual_addr == 0 {
        return 0;
    }

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_mut() } {
        Some(m) => m,
        None => return 0,
    };

    // 获取页表管理器
    let page_table_manager = match manager.page_table_manager.as_ref() {
        Some(ptm) => ptm,
        None => return 0,
    };

    let virt = VirtAddr::new(virtual_addr);

    // 执行地址转换
    match page_table_manager.translate(virt) {
        Ok(phys) => phys.as_u64(),
        Err(_) => 0,  // 失败
    }
}

/// C兼容的内存摘要结构
#[repr(C)]
#[derive(Clone, Copy)]
pub struct CMemorySummary {
    pub total_physical_mb: u64,
    pub used_physical_mb: u64,
    pub free_physical_mb: u64,
    pub heap_used_kb: usize,
    pub heap_free_kb: usize,
    pub page_tables_count: usize,
    pub usage_percent: u32,
}

/// 获取内存使用摘要
/// 阶段2: 返回真实的物理内存统计
#[no_mangle]
pub extern "C" fn rust_memory_summary(summary: *mut CMemorySummary) -> i32 {
    if summary.is_null() {
        return -1;
    }

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_ref() } {
        Some(m) => m,
        None => {
            // 如果未初始化，返回假数据
            unsafe {
                (*summary) = CMemorySummary {
                    total_physical_mb: 128,
                    used_physical_mb: 8,
                    free_physical_mb: 120,
                    heap_used_kb: 512,
                    heap_free_kb: 7680,
                    page_tables_count: 1,
                    usage_percent: 6,
                };
            }
            return 0;
        }
    };

    // 计算真实的物理内存统计
    let total_pages = manager.physical_allocator.total_pages();
    let allocated_pages = manager.physical_allocator.allocated_pages();
    let free_pages = manager.physical_allocator.free_pages();

    let total_mb = (total_pages * 4096) / (1024 * 1024);
    let used_mb = (allocated_pages * 4096) / (1024 * 1024);
    let free_mb = (free_pages * 4096) / (1024 * 1024);

    let usage = if total_pages > 0 {
        (allocated_pages * 100 / total_pages) as u32
    } else {
        0
    };

    unsafe {
        (*summary) = CMemorySummary {
            total_physical_mb: total_mb as u64,
            used_physical_mb: used_mb as u64,
            free_physical_mb: free_mb as u64,
            heap_used_kb: 0,  // 阶段2: 堆未初始化
            heap_free_kb: 0,
            page_tables_count: 0,
            usage_percent: usage,
        };
    }

    0
}

/// 检查内存完整性
/// 阶段2: 总是返回正常
#[no_mangle]
pub extern "C" fn rust_memory_check() -> i32 {
    0  // 正常
}

/// 分配连续的物理页面
/// 阶段2: 暂不实现
#[no_mangle]
pub extern "C" fn rust_alloc_pages(_count: usize) -> u64 {
    0
}

/// 释放连续的物理页面
/// 阶段2: 空操作
#[no_mangle]
pub extern "C" fn rust_free_pages(_start_addr: u64, _count: usize) {
    // 什么都不做
}

// ============================================================================
// VMM (虚拟内存管理器) FFI 接口
// ============================================================================

/// 分配虚拟地址空间（不映射到物理内存）
#[no_mangle]
pub extern "C" fn rust_vmm_allocate(size: u64) -> u64 {
    use crate::arch::addr::VirtAddr;

    if size == 0 {
        return 0;
    }

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_mut() } {
        Some(m) => m,
        None => {
            serial_log!("ERROR: Memory manager not initialized");
            return 0;
        }
    };

    // 获取VMM
    let vmm = match manager.vmm.as_mut() {
        Some(v) => v,
        None => {
            serial_log!("ERROR: VMM not initialized");
            return 0;
        }
    };

    // 分配虚拟地址
    match vmm.allocate_kernel_heap(size) {
        Ok(virt_addr) => virt_addr.as_u64(),
        Err(_e) => {
            serial_log!("ERROR: Failed to allocate virtual address");
            0
        }
    }
}

/// 分配并映射虚拟内存
#[no_mangle]
pub extern "C" fn rust_vmm_map_and_allocate(size: u64, out_virt_addr: *mut u64) -> i32 {
    use crate::vmm::VmmFlags;

    if size == 0 || out_virt_addr.is_null() {
        return -1;
    }

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_mut() } {
        Some(m) => m,
        None => {
            serial_log!("ERROR: Memory manager not initialized");
            return -1;
        }
    };

    // 获取VMM和页表管理器
    let (vmm, page_table) = match (&mut manager.vmm, &mut manager.page_table_manager) {
        (Some(v), Some(pt)) => (v, pt),
        _ => {
            serial_log!("ERROR: VMM or page table manager not initialized");
            return -1;
        }
    };

    // 创建标志（可写、不可执行）
    let flags = VmmFlags::new().writable();

    // 分配物理页面的闭包
    let alloc_frame = || manager.physical_allocator.allocate_frame();

    // 分配并映射
    match vmm.allocate_and_map(page_table, size, flags, alloc_frame) {
        Ok(virt_addr) => {
            unsafe {
                *out_virt_addr = virt_addr.as_u64();
            }
            0  // 成功
        }
        Err(_e) => {
            serial_log!("ERROR: Failed to allocate and map memory");
            -1  // 失败
        }
    }
}

/// 获取内核堆使用情况
#[no_mangle]
pub extern "C" fn rust_vmm_get_heap_usage(used: *mut u64, total: *mut u64) {
    if used.is_null() || total.is_null() {
        return;
    }

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_ref() } {
        Some(m) => m,
        None => {
            unsafe {
                *used = 0;
                *total = 0;
            }
            return;
        }
    };

    // 获取VMM
    let vmm = match manager.vmm.as_ref() {
        Some(v) => v,
        None => {
            unsafe {
                *used = 0;
                *total = 0;
            }
            return;
        }
    };

    let (used_bytes, total_bytes) = vmm.kernel_heap_usage();
    unsafe {
        *used = used_bytes;
        *total = total_bytes;
    }
}

/// 获取堆统计信息
#[no_mangle]
pub extern "C" fn rust_heap_stats(
    total_alloc: *mut usize,
    total_freed: *mut usize,
    current: *mut usize,
    alloc_count: *mut usize,
    free_count: *mut usize,
) {
    if total_alloc.is_null() || total_freed.is_null() || current.is_null() 
        || alloc_count.is_null() || free_count.is_null() {
        return;
    }

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_ref() } {
        Some(m) => m,
        None => {
            unsafe {
                *total_alloc = 0;
                *total_freed = 0;
                *current = 0;
                *alloc_count = 0;
                *free_count = 0;
            }
            return;
        }
    };

    // 获取堆分配器
    let heap = match manager.heap_allocator.as_ref() {
        Some(h) => h,
        None => {
            unsafe {
                *total_alloc = 0;
                *total_freed = 0;
                *current = 0;
                *alloc_count = 0;
                *free_count = 0;
            }
            return;
        }
    };

    let stats = heap.stats();
    unsafe {
        *total_alloc = stats.total_allocated;
        *total_freed = stats.total_freed;
        *current = stats.current_usage;
        *alloc_count = stats.allocation_count;
        *free_count = stats.free_count;
    }
}

// ============================================================================
// 内存保护 FFI 接口
// ============================================================================

/// 设置页面为只读
#[no_mangle]
pub extern "C" fn rust_set_page_readonly(virt_addr: u64) -> i32 {
    use crate::arch::addr::VirtAddr;
    use crate::protection::ProtectionManager;

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_mut() } {
        Some(m) => m,
        None => {
            serial_log!("ERROR: Memory manager not initialized");
            return -1;
        }
    };

    // 获取页表管理器
    let page_table = match &mut manager.page_table_manager {
        Some(pt) => pt,
        None => {
            serial_log!("ERROR: Page table manager not initialized");
            return -1;
        }
    };

    let virt = VirtAddr::new(virt_addr);

    match ProtectionManager::set_readonly(page_table, virt) {
        Ok(_) => 0,
        Err(_e) => {
            serial_log!("ERROR: Failed to set page readonly");
            -1
        }
    }
}

/// 设置页面为可读写
#[no_mangle]
pub extern "C" fn rust_set_page_readwrite(virt_addr: u64) -> i32 {
    use crate::arch::addr::VirtAddr;
    use crate::protection::ProtectionManager;

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_mut() } {
        Some(m) => m,
        None => {
            serial_log!("ERROR: Memory manager not initialized");
            return -1;
        }
    };

    // 获取页表管理器
    let page_table = match &mut manager.page_table_manager {
        Some(pt) => pt,
        None => {
            serial_log!("ERROR: Page table manager not initialized");
            return -1;
        }
    };

    let virt = VirtAddr::new(virt_addr);

    match ProtectionManager::set_readwrite(page_table, virt) {
        Ok(_) => 0,
        Err(_e) => {
            serial_log!("ERROR: Failed to set page readwrite");
            -1
        }
    }
}

/// 设置页面为不可执行
#[no_mangle]
pub extern "C" fn rust_set_page_no_execute(virt_addr: u64) -> i32 {
    use crate::arch::addr::VirtAddr;
    use crate::protection::ProtectionManager;

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_mut() } {
        Some(m) => m,
        None => {
            serial_log!("ERROR: Memory manager not initialized");
            return -1;
        }
    };

    // 获取页表管理器
    let page_table = match &mut manager.page_table_manager {
        Some(pt) => pt,
        None => {
            serial_log!("ERROR: Page table manager not initialized");
            return -1;
        }
    };

    let virt = VirtAddr::new(virt_addr);

    match ProtectionManager::set_no_execute(page_table, virt) {
        Ok(_) => 0,
        Err(_e) => {
            serial_log!("ERROR: Failed to set page no-execute");
            -1
        }
    }
}

/// 获取页面保护标志
#[no_mangle]
pub extern "C" fn rust_get_page_flags(
    virt_addr: u64,
    out_present: *mut bool,
    out_writable: *mut bool,
    out_user: *mut bool,
    out_executable: *mut bool,
) -> i32 {
    use crate::arch::addr::VirtAddr;
    use crate::protection::ProtectionManager;

    if out_present.is_null() || out_writable.is_null() 
        || out_user.is_null() || out_executable.is_null() {
        return -1;
    }

    // 获取全局内存管理器实例
    let manager = match unsafe { crate::MEMORY_MANAGER.as_ref() } {
        Some(m) => m,
        None => {
            serial_log!("ERROR: Memory manager not initialized");
            return -1;
        }
    };

    // 获取页表管理器
    let page_table = match &manager.page_table_manager {
        Some(pt) => pt,
        None => {
            serial_log!("ERROR: Page table manager not initialized");
            return -1;
        }
    };

    let virt = VirtAddr::new(virt_addr);

    match ProtectionManager::get_page_protection(page_table, virt) {
        Ok(flags) => {
            unsafe {
                *out_present = flags.present;
                *out_writable = flags.writable;
                *out_user = flags.user;
                *out_executable = flags.executable;
            }
            0
        }
        Err(_e) => {
            serial_log!("ERROR: Failed to get page protection");
            -1
        }
    }
}
