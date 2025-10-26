// Boruix OS 显示系统互斥锁
// 保护显示函数在多进程环境下的线程安全

#include "kernel/types.h"

// 简单的自旋锁实现
static volatile uint32_t display_lock = 0;

// 获取显示锁
void display_acquire_lock(void) {
    // 禁用中断，防止死锁
    uint64_t flags;
    __asm__ volatile(
        "pushfq\n"
        "pop %0\n"
        "cli"
        : "=r"(flags)
    );
    
    // 自旋等待锁
    while (__sync_lock_test_and_set(&display_lock, 1)) {
        __asm__ volatile("pause");
    }
    
    // 保存中断状态（简化实现，实际应该保存到栈）
    (void)flags;
}

// 释放显示锁
void display_release_lock(void) {
    __sync_lock_release(&display_lock);
    
    // 恢复中断
    __asm__ volatile("sti");
}

