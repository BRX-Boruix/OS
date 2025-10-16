// Boruix OS 中断处理头文件
// 处理硬件中断

#ifndef BORUIX_INTERRUPT_H
#define BORUIX_INTERRUPT_H

// 中断向量定义
#define IRQ0_TIMER 0x20
#define IRQ1_KEYBOARD 0x21

// 中断描述符表相关
#define IDT_SIZE 256
#define IDT_BASE 0x00000000

// 中断门类型
#define IDT_INTERRUPT_GATE 0x8E
#define IDT_TRAP_GATE 0x8F

// 中断函数声明
void interrupt_init(void);
void interrupt_handler_keyboard(void);
void interrupt_handler_timer(void);
void interrupt_enable(void);
void interrupt_disable(void);

// 中断描述符结构
typedef struct {
    unsigned short offset_low;
    unsigned short selector;
    unsigned char zero;
    unsigned char type;
    unsigned short offset_high;
} __attribute__((packed)) idt_entry_t;

// IDT指针结构
typedef struct {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)) idt_ptr_t;

#endif // BORUIX_INTERRUPT_H
