//! HHDM (Higher Half Direct Map) 支持
//! 提供物理地址到虚拟地址的直接映射转换

use crate::arch::addr::{PhysAddr, VirtAddr};
use core::sync::atomic::{AtomicU64, Ordering};

/// 全局HHDM偏移量
static HHDM_OFFSET: AtomicU64 = AtomicU64::new(0);

/// 设置HHDM偏移量
/// 必须在初始化内存管理器之前调用
pub fn set_offset(offset: u64) {
    HHDM_OFFSET.store(offset, Ordering::SeqCst);
}

/// 获取HHDM偏移量
pub fn get_offset() -> u64 {
    HHDM_OFFSET.load(Ordering::SeqCst)
}

/// 将物理地址转换为虚拟地址（通过HHDM）
pub fn phys_to_virt(phys_addr: PhysAddr) -> VirtAddr {
    let offset = get_offset();
    VirtAddr::new(phys_addr.as_u64() + offset)
}

/// 将虚拟地址转换为物理地址（通过HHDM）
/// 注意：只对HHDM区域的地址有效
pub fn virt_to_phys(virt_addr: VirtAddr) -> Option<PhysAddr> {
    let offset = get_offset();
    let addr = virt_addr.as_u64();
    
    if addr >= offset {
        Some(PhysAddr::new(addr - offset))
    } else {
        None
    }
}

/// 检查HHDM是否已初始化
pub fn is_initialized() -> bool {
    get_offset() != 0
}

