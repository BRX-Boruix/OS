// Boruix OS i386中断处理程序

#include "kernel/types.h"
#include "drivers/display.h"
#include "drivers/timer.h"
#include "drivers/keyboard.h"

// 外部函数
extern void pic_send_eoi(uint8_t irq);

// 中断寄存器状态结构（与汇编压栈顺序对应）
typedef struct {
    uint32_t ds;                    // 数据段选择子
    uint32_t edi, esi, ebp, esp;    // pusha压入的寄存器
    uint32_t ebx, edx, ecx, eax;
    uint32_t int_no, err_code;      // 中断号和错误码
    uint32_t eip, cs, eflags;       // CPU自动压栈
    uint32_t useresp, ss;           // 用户模式时的栈信息
} __attribute__((packed)) registers_t;

// 中断计数器（用于调试）
static uint32_t interrupt_counts[256] = {0};

// 异常名称
static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Security Exception",
    "Reserved"
};

// ISR处理函数 - 处理CPU异常（0-31）
void isr_handler(registers_t* regs) {
    uint32_t int_no = regs->int_no;
    
    // 增加计数
    interrupt_counts[int_no]++;
    
    // 显示异常信息
    if (int_no < 32) {
        print_string("\n========================================\n");
        print_string("[EXCEPTION] ");
        print_string(exception_messages[int_no]);
        print_string("\n");
        print_string("INT: ");
        print_dec(int_no);
        print_string("  ERR: ");
        print_hex(regs->err_code);
        print_string("\n");
        print_string("EIP: ");
        print_hex(regs->eip);
        print_string("  CS: ");
        print_hex(regs->cs);
        print_string("\n");
        print_string("EFLAGS: ");
        print_hex(regs->eflags);
        print_string("\n");
        print_string("========================================\n");
        
        // 异常后暂停
        print_string("System halted.\n");
        while(1) {
            __asm__ volatile("hlt");
        }
    }
}

// IRQ处理函数 - 处理硬件中断（32-47）
void irq_handler(registers_t* regs) {
    uint32_t int_no = regs->int_no;
    
    // 增加计数
    interrupt_counts[int_no]++;
    
    // 调用特定IRQ处理程序
    uint8_t irq = int_no - 32;
    
    switch (irq) {
        case 0:  // IRQ0 - Timer
            timer_irq_handler();
            break;
            
        case 1:  // IRQ1 - Keyboard
            keyboard_irq_handler();
            break;
            
        default:
            // 其他IRQ暂不处理
            break;
    }
    
    // 发送EOI
    pic_send_eoi(irq);
}

// 获取中断计数
uint32_t get_interrupt_count(uint8_t int_no) {
    if (int_no < 256) {
        return interrupt_counts[int_no];
    }
    return 0;
}
