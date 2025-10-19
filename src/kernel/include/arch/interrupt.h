// Boruix OS 中断处理头文件
// 处理硬件中断

#ifndef BORUIX_INTERRUPT_H
#define BORUIX_INTERRUPT_H

// 包含x86_64架构头文件
#include "arch/x86_64.h"

// 中断函数声明
void interrupt_init(void);
void interrupt_handler_keyboard(void);
void interrupt_handler_timer(void);
void interrupt_enable(void);
void interrupt_disable(void);

#endif // BORUIX_INTERRUPT_H
