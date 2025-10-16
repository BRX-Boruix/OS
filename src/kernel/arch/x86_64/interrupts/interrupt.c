// Boruix OS 中断处理实现 (x86_64版本)
// 处理硬件中断

#include "arch/interrupt.h"
#include "drivers/keyboard.h"
#include "kernel/kernel.h"

// 中断描述符表
static idt_entry_t idt[IDT_SIZE];
static idt_ptr_t idt_ptr;

// 中断处理程序数组
static void (*interrupt_handlers[IDT_SIZE])(void);

// 设置中断描述符 (x86_64版本)
static void idt_set_gate(unsigned char num, uint64_t base, unsigned short sel, unsigned char flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_mid = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].selector = sel;
    idt[num].ist = 0;  // 不使用中断栈表
    idt[num].type = flags;
    idt[num].reserved = 0;
}

// 加载IDT
static void idt_load(void) {
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_SIZE - 1;
    idt_ptr.base = (uint64_t)&idt;
    
    __asm__ volatile ("lidt %0" : : "m" (idt_ptr));
}

// 初始化中断
void interrupt_init(void) {
    // 清零IDT
    for (int i = 0; i < IDT_SIZE; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    // 设置键盘中断 (IRQ1 -> INT 0x21)
    idt_set_gate(IRQ1_KEYBOARD, (uint64_t)interrupt_handler_keyboard, 0x08, IDT_INTERRUPT_GATE);
    
    // 设置定时器中断 (IRQ0 -> INT 0x20)
    idt_set_gate(IRQ0_TIMER, (uint64_t)interrupt_handler_timer, 0x08, IDT_INTERRUPT_GATE);
    
    // 加载IDT
    idt_load();
    
    // 初始化键盘
    keyboard_init();
    
    // 启用中断
    interrupt_enable();
}

// 键盘中断处理程序
void interrupt_handler_keyboard(void) {
    // 调用键盘中断处理
    keyboard_interrupt_handler();
    
    // 发送EOI (End of Interrupt)
    __asm__ volatile ("movb $0x20, %al");
    __asm__ volatile ("outb %al, $0x20");
}

// 定时器中断处理程序
void interrupt_handler_timer(void) {
    // 简单的定时器处理
    // 这里可以添加定时器相关的功能
    
    // 发送EOI
    __asm__ volatile ("movb $0x20, %al");
    __asm__ volatile ("outb %al, $0x20");
}

// 启用中断
void interrupt_enable(void) {
    __asm__ volatile ("sti");
}

// 禁用中断
void interrupt_disable(void) {
    __asm__ volatile ("cli");
}
