// Boruix OS 中断系统头文件
// 提供中断处理的基础定义

#ifndef BORUIX_INTERRUPT_H
#define BORUIX_INTERRUPT_H

#include "kernel/types.h"

// 中断向量定义
#define IRQ_BASE 32  // IRQ基址（重映射后）

// 中断启用/禁用函数
static inline void interrupts_enable(void) {
    __asm__ volatile("sti");
}

static inline void interrupts_disable(void) {
    __asm__ volatile("cli");
}

static inline bool interrupts_enabled(void) {
#ifdef __x86_64__
    uint64_t flags;
    __asm__ volatile("pushfq; pop %0" : "=r"(flags));
#else
    uint32_t flags;
    __asm__ volatile("pushf; pop %0" : "=r"(flags));
#endif
    return (flags & 0x200) != 0;
}

// 中断初始化函数（由架构特定代码实现）
void interrupt_init(void);

#endif // BORUIX_INTERRUPT_H
