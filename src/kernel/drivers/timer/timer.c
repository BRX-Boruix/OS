// Boruix OS 定时器驱动实现

#include "drivers/timer.h"

// PIT端口
#define PIT_CHANNEL0 0x40
#define PIT_COMMAND 0x43

// I/O端口操作
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

// 系统时钟滴答计数
volatile uint32_t system_ticks = 0;

// 定时器中断处理
void timer_irq_handler(void) {
    system_ticks++;
}

// 初始化定时器
void timer_init(uint32_t frequency) {
    // 计算除数
    uint32_t divisor = 1193180 / frequency;
    
    // 发送命令字节
    // 频道0, lohi模式, 方波生成器
    outb(PIT_COMMAND, 0x36);
    
    // 发送频率除数
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}

// 获取系统运行时间（秒）
uint32_t timer_get_seconds(void) {
    return system_ticks / TIMER_FREQ_HZ;
}
