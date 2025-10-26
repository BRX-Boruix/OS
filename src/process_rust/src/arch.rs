//! x86_64架构相关定义和常量

#![allow(dead_code)]

/// 内核栈大小 (16KB)
pub const KERNEL_STACK_SIZE: usize = 16 * 1024;

/// 用户栈大小 (16KB)
pub const USER_STACK_SIZE: usize = 16 * 1024;

/// 页面大小
pub const PAGE_SIZE: usize = 4096;

/// 内核代码段选择子
pub const KERNEL_CS: u64 = 0x08;

/// 内核数据段选择子
pub const KERNEL_DS: u64 = 0x10;

/// 用户代码段选择子 (RPL=3)
pub const USER_CS: u64 = 0x1B;

/// 用户数据段选择子 (RPL=3)
pub const USER_DS: u64 = 0x23;

/// RFLAGS中断标志位
pub const RFLAGS_IF: u64 = 0x200;

/// RFLAGS保留位
pub const RFLAGS_RESERVED: u64 = 0x2;

/// 默认RFLAGS值 (IF=1, 保留位=1)
pub const DEFAULT_RFLAGS: u64 = RFLAGS_IF | RFLAGS_RESERVED;

/// 读取时间戳计数器
#[inline]
pub fn read_tsc() -> u64 {
    let low: u32;
    let high: u32;
    unsafe {
        core::arch::asm!(
            "rdtsc",
            out("eax") low,
            out("edx") high,
            options(nomem, nostack, preserves_flags)
        );
    }
    ((high as u64) << 32) | (low as u64)
}

/// 禁用中断并返回之前的状态
#[inline]
pub fn disable_interrupts() -> bool {
    let flags: u64;
    unsafe {
        core::arch::asm!(
            "pushfq",
            "pop {flags}",
            "cli",
            flags = out(reg) flags,
            options(nomem, preserves_flags)
        );
    }
    (flags & RFLAGS_IF) != 0
}

/// 启用中断
#[inline]
pub fn enable_interrupts() {
    unsafe {
        core::arch::asm!("sti", options(nomem, nostack, preserves_flags));
    }
}

/// 恢复中断状态
#[inline]
pub fn restore_interrupts(enabled: bool) {
    if enabled {
        enable_interrupts();
    }
}

/// 内存屏障
#[inline]
pub fn memory_barrier() {
    unsafe {
        core::arch::asm!("mfence", options(nomem, nostack, preserves_flags));
    }
}

/// CPU暂停指令 (用于自旋锁)
#[inline]
pub fn cpu_pause() {
    unsafe {
        core::arch::asm!("pause", options(nomem, nostack, preserves_flags));
    }
}

/// HLT指令 (等待中断)
#[inline]
pub fn halt() {
    unsafe {
        core::arch::asm!("hlt", options(nomem, nostack, preserves_flags));
    }
}

/// 获取当前栈指针
#[inline]
pub fn get_stack_pointer() -> usize {
    let sp: usize;
    unsafe {
        core::arch::asm!(
            "mov {sp}, rsp",
            sp = out(reg) sp,
            options(nomem, nostack, preserves_flags)
        );
    }
    sp
}

/// 设置栈指针
#[inline]
pub unsafe fn set_stack_pointer(sp: usize) {
    core::arch::asm!(
        "mov rsp, {sp}",
        sp = in(reg) sp,
        options(nomem, nostack, preserves_flags)
    );
}

