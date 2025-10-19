// Boruix OS x86_64中断优先级管理
// 实现中断优先级控制和管理

#include "kernel/types.h"
#include "kernel/interrupt.h"

// 中断优先级级别定义
#define IRQ_PRIORITY_CRITICAL   0   // 关键优先级（最高）
#define IRQ_PRIORITY_HIGH       1   // 高优先级
#define IRQ_PRIORITY_NORMAL     2   // 普通优先级
#define IRQ_PRIORITY_LOW        3   // 低优先级
#define IRQ_PRIORITY_DISABLED   255 // 禁用

// 中断优先级表（0-15对应IRQ0-15）
static uint8_t irq_priorities[16] = {
    IRQ_PRIORITY_CRITICAL,  // IRQ0  - Timer（定时器，最高优先级）
    IRQ_PRIORITY_HIGH,      // IRQ1  - Keyboard（键盘）
    IRQ_PRIORITY_NORMAL,    // IRQ2  - Cascade（级联）
    IRQ_PRIORITY_NORMAL,    // IRQ3  - COM2
    IRQ_PRIORITY_NORMAL,    // IRQ4  - COM1
    IRQ_PRIORITY_LOW,       // IRQ5  - LPT2
    IRQ_PRIORITY_NORMAL,    // IRQ6  - Floppy
    IRQ_PRIORITY_LOW,       // IRQ7  - LPT1
    IRQ_PRIORITY_NORMAL,    // IRQ8  - RTC
    IRQ_PRIORITY_NORMAL,    // IRQ9  - Free
    IRQ_PRIORITY_NORMAL,    // IRQ10 - Free
    IRQ_PRIORITY_NORMAL,    // IRQ11 - Free
    IRQ_PRIORITY_HIGH,      // IRQ12 - PS/2 Mouse
    IRQ_PRIORITY_NORMAL,    // IRQ13 - FPU
    IRQ_PRIORITY_NORMAL,    // IRQ14 - Primary ATA
    IRQ_PRIORITY_NORMAL     // IRQ15 - Secondary ATA
};

// 当前中断嵌套级别
static uint8_t current_interrupt_level = IRQ_PRIORITY_DISABLED;

// 中断嵌套计数
static uint32_t interrupt_nesting_count = 0;

// 被阻塞的中断计数
static uint64_t blocked_interrupt_counts[16] = {0};

// 初始化中断优先级系统
void irq_priority_init(void) {
    current_interrupt_level = IRQ_PRIORITY_DISABLED;
    interrupt_nesting_count = 0;
    
    // 清空阻塞计数
    for (int i = 0; i < 16; i++) {
        blocked_interrupt_counts[i] = 0;
    }
}

// 设置IRQ优先级
void irq_set_priority(uint8_t irq, uint8_t priority) {
    if (irq < 16 && priority <= IRQ_PRIORITY_LOW) {
        irq_priorities[irq] = priority;
    }
}

// 获取IRQ优先级
uint8_t irq_get_priority(uint8_t irq) {
    if (irq < 16) {
        return irq_priorities[irq];
    }
    return IRQ_PRIORITY_DISABLED;
}

// 检查中断是否应该被允许执行
// 返回true表示允许，false表示应该被阻塞
bool irq_should_execute(uint8_t irq) {
    if (irq >= 16) {
        return false;
    }
    
    uint8_t priority = irq_priorities[irq];
    
    // 禁用的中断永远不执行
    if (priority == IRQ_PRIORITY_DISABLED) {
        return false;
    }
    
    // 如果没有中断正在执行，允许所有中断
    if (current_interrupt_level == IRQ_PRIORITY_DISABLED) {
        return true;
    }
    
    // 只有优先级更高（数值更小）的中断才能打断当前中断
    if (priority < current_interrupt_level) {
        return true;
    }
    
    // 否则阻塞该中断
    blocked_interrupt_counts[irq]++;
    return false;
}

// 进入中断处理（在IRQ处理程序开始时调用）
void irq_enter(uint8_t irq) {
    if (irq < 16) {
        current_interrupt_level = irq_priorities[irq];
        interrupt_nesting_count++;
    }
}

// 退出中断处理（在IRQ处理程序结束时调用）
void irq_exit(void) {
    if (interrupt_nesting_count > 0) {
        interrupt_nesting_count--;
    }
    
    // 如果没有嵌套中断，恢复到禁用状态
    if (interrupt_nesting_count == 0) {
        current_interrupt_level = IRQ_PRIORITY_DISABLED;
    }
}

// 获取当前中断级别
uint8_t irq_get_current_level(void) {
    return current_interrupt_level;
}

// 获取中断嵌套计数
uint32_t irq_get_nesting_count(void) {
    return interrupt_nesting_count;
}

// 获取被阻塞的中断计数
uint64_t irq_get_blocked_count(uint8_t irq) {
    if (irq < 16) {
        return blocked_interrupt_counts[irq];
    }
    return 0;
}

// 获取优先级名称（用于显示）
const char* irq_get_priority_name(uint8_t priority) {
    switch (priority) {
        case IRQ_PRIORITY_CRITICAL:
            return "Critical";
        case IRQ_PRIORITY_HIGH:
            return "High";
        case IRQ_PRIORITY_NORMAL:
            return "Normal";
        case IRQ_PRIORITY_LOW:
            return "Low";
        case IRQ_PRIORITY_DISABLED:
            return "Disabled";
        default:
            return "Unknown";
    }
}

// 禁用IRQ
void irq_disable(uint8_t irq) {
    if (irq < 16) {
        irq_priorities[irq] = IRQ_PRIORITY_DISABLED;
    }
}

// 启用IRQ（恢复到默认优先级）
void irq_enable(uint8_t irq) {
    if (irq >= 16) {
        return;
    }
    
    // 根据IRQ类型设置默认优先级
    switch (irq) {
        case 0:  // Timer
            irq_priorities[irq] = IRQ_PRIORITY_CRITICAL;
            break;
        case 1:  // Keyboard
        case 12: // Mouse
            irq_priorities[irq] = IRQ_PRIORITY_HIGH;
            break;
        case 5:  // LPT2
        case 7:  // LPT1
            irq_priorities[irq] = IRQ_PRIORITY_LOW;
            break;
        default:
            irq_priorities[irq] = IRQ_PRIORITY_NORMAL;
            break;
    }
}

// 重置所有优先级到默认值
void irq_reset_priorities(void) {
    for (int i = 0; i < 16; i++) {
        irq_enable(i);
    }
}

