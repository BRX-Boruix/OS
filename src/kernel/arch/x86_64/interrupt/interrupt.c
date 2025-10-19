// Boruix OS x86_64中断系统初始化

#include "kernel/interrupt.h"
#include "drivers/display.h"
#include "drivers/timer.h"

void idt_init(void);
void pic_init(void);
extern void pic_clear_mask(uint8_t irq);

void interrupt_init(void) {
    print_string("[INT] Initializing interrupt system (x86_64)...\n");
    
    interrupts_disable();
    
    idt_init();
    
    pic_init();
    print_string("[PIC] Programmable Interrupt Controller initialized\n");
    
    // 初始化中断优先级系统
    irq_priority_init();
    print_string("[IRQ] Interrupt priority system initialized\n");
    
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
