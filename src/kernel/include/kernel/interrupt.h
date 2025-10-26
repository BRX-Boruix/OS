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

// 进程切换相关函数
void enable_process_switching(void);
void disable_process_switching(void);
void idt_set_timer_handler_with_switch(void);

// 首次进程切换
void switch_to_first_process(uint32_t pid);

// 中断优先级管理函数
void irq_priority_init(void);
void irq_set_priority(uint8_t irq, uint8_t priority);
uint8_t irq_get_priority(uint8_t irq);
bool irq_should_execute(uint8_t irq);
void irq_enter(uint8_t irq);
void irq_exit(void);
uint8_t irq_get_current_level(void);
uint32_t irq_get_nesting_count(void);
uint64_t irq_get_blocked_count(uint8_t irq);
const char* irq_get_priority_name(uint8_t priority);
void irq_disable(uint8_t irq);
void irq_enable(uint8_t irq);
void irq_reset_priorities(void);

// 优先级级别定义
#define IRQ_PRIORITY_CRITICAL   0
#define IRQ_PRIORITY_HIGH       1
#define IRQ_PRIORITY_NORMAL     2
#define IRQ_PRIORITY_LOW        3
#define IRQ_PRIORITY_DISABLED   255

#endif // BORUIX_INTERRUPT_H
