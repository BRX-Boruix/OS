// Boruix OS i386中断系统初始化

#include "kernel/interrupt.h"
#include "drivers/display.h"
#include "drivers/timer.h"

// 外部函数声明
void idt_init(void);
void pic_init(void);
extern void pic_clear_mask(uint8_t irq);

// 初始化整个中断系统
void interrupt_init(void) {
    print_string("[INT] Initializing interrupt system (i386)...\n");
    
    // 禁用中断
    interrupts_disable();
    
    // 初始化IDT
    idt_init();
    
    // 初始化PIC
    pic_init();
    print_string("[PIC] Programmable Interrupt Controller initialized\n");
    
    // 初始化定时器
    timer_init(TIMER_FREQ_HZ);
    print_string("[TIMER] System timer initialized (");
    print_dec(TIMER_FREQ_HZ);
    print_string(" Hz)\n");
    
    // 启用IRQ0（定时器）和IRQ1（键盘）
    pic_clear_mask(0);
    pic_clear_mask(1);
    
    print_string("[INT] Interrupt system initialized\n");
    // 注意：不自动启用中断，等待shell准备好
}
