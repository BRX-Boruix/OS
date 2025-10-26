// Boruix OS 定时器驱动头文件

#ifndef BORUIX_TIMER_H
#define BORUIX_TIMER_H

#include "kernel/types.h"

// PIT频率
#define TIMER_FREQ_HZ 100  // 100Hz = 每秒100次中断

// 全局时钟滴答计数
extern volatile uint32_t system_ticks;

// 初始化定时器
void timer_init(uint32_t frequency);

// 获取系统运行时间（秒）
uint32_t timer_get_seconds(void);

// 定时器中断处理
void timer_irq_handler(void);

// 设置调度器初始化状态
void timer_set_scheduler_initialized(bool initialized);

#endif // BORUIX_TIMER_H
