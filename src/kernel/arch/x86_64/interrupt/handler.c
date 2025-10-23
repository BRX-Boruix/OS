// Boruix OS x86_64中断处理程序

#include "kernel/types.h"
#include "kernel/interrupt.h"
#include "drivers/display.h"
#include "drivers/timer.h"
#include "drivers/keyboard.h"

extern void pic_send_eoi(uint8_t irq);

// 中断寄存器状态结构（与汇编压栈顺序对应）
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) registers_t;

static uint64_t interrupt_counts[256] = {0};

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

// 双重错误专用处理器
static void double_fault_handler(registers_t* regs) {
    // 禁用中断
    __asm__ volatile("cli");
    
    print_string("\n");
    print_string("###########################################################\n");
    print_string("#                                                         #\n");
    print_string("#              DOUBLE FAULT DETECTED!                     #\n");
    print_string("#                                                         #\n");
    print_string("#  A double fault occurred - this means an exception     #\n");
    print_string("#  happened while handling another exception.            #\n");
    print_string("#                                                         #\n");
    print_string("#  Using independent stack (IST1) for safe handling.     #\n");
    print_string("#                                                         #\n");
    print_string("###########################################################\n");
    print_string("\n");
    
    print_string("[DOUBLE FAULT] System State Dump:\n");
    print_string("  Error Code: 0x");
    print_hex((uint32_t)regs->err_code);
    print_string("\n");
    
    print_string("  RIP (Instruction): 0x");
    print_hex((uint32_t)(regs->rip >> 32));
    print_hex((uint32_t)regs->rip);
    print_string("\n");
    
    print_string("  RSP (Stack):       0x");
    print_hex((uint32_t)(regs->rsp >> 32));
    print_hex((uint32_t)regs->rsp);
    print_string("\n");
    
    print_string("  RBP (Base):        0x");
    print_hex((uint32_t)(regs->rbp >> 32));
    print_hex((uint32_t)regs->rbp);
    print_string("\n");
    
    print_string("  CS:  0x");
    print_hex((uint32_t)regs->cs);
    print_string("  SS:  0x");
    print_hex((uint32_t)regs->ss);
    print_string("\n");
    
    print_string("  RFLAGS: 0x");
    print_hex((uint32_t)(regs->rflags >> 32));
    print_hex((uint32_t)regs->rflags);
    print_string("\n");
    
    print_string("\n");
    print_string("[DOUBLE FAULT] General Purpose Registers:\n");
    print_string("  RAX: 0x");
    print_hex((uint32_t)(regs->rax >> 32));
    print_hex((uint32_t)regs->rax);
    print_string("  RBX: 0x");
    print_hex((uint32_t)(regs->rbx >> 32));
    print_hex((uint32_t)regs->rbx);
    print_string("\n");
    
    print_string("  RCX: 0x");
    print_hex((uint32_t)(regs->rcx >> 32));
    print_hex((uint32_t)regs->rcx);
    print_string("  RDX: 0x");
    print_hex((uint32_t)(regs->rdx >> 32));
    print_hex((uint32_t)regs->rdx);
    print_string("\n");
    
    print_string("  RSI: 0x");
    print_hex((uint32_t)(regs->rsi >> 32));
    print_hex((uint32_t)regs->rsi);
    print_string("  RDI: 0x");
    print_hex((uint32_t)(regs->rdi >> 32));
    print_hex((uint32_t)regs->rdi);
    print_string("\n");
    
    print_string("\n");
    print_string("[DOUBLE FAULT] Extended Registers:\n");
    print_string("  R8:  0x");
    print_hex((uint32_t)(regs->r8 >> 32));
    print_hex((uint32_t)regs->r8);
    print_string("  R9:  0x");
    print_hex((uint32_t)(regs->r9 >> 32));
    print_hex((uint32_t)regs->r9);
    print_string("\n");
    
    print_string("  R10: 0x");
    print_hex((uint32_t)(regs->r10 >> 32));
    print_hex((uint32_t)regs->r10);
    print_string("  R11: 0x");
    print_hex((uint32_t)(regs->r11 >> 32));
    print_hex((uint32_t)regs->r11);
    print_string("\n");
    
    print_string("  R12: 0x");
    print_hex((uint32_t)(regs->r12 >> 32));
    print_hex((uint32_t)regs->r12);
    print_string("  R13: 0x");
    print_hex((uint32_t)(regs->r13 >> 32));
    print_hex((uint32_t)regs->r13);
    print_string("\n");
    
    print_string("  R14: 0x");
    print_hex((uint32_t)(regs->r14 >> 32));
    print_hex((uint32_t)regs->r14);
    print_string("  R15: 0x");
    print_hex((uint32_t)(regs->r15 >> 32));
    print_hex((uint32_t)regs->r15);
    print_string("\n");
    
    print_string("\n");
    print_string("###########################################################\n");
    print_string("# System cannot recover from double fault.                #\n");
    print_string("# Please reboot the system.                               #\n");
    print_string("###########################################################\n");
    
    // 无限循环，等待重启
    while(1) {
        __asm__ volatile("hlt");
    }
}

// ISR处理函数
void isr_handler(registers_t* regs) {
    uint32_t int_no = (uint32_t)regs->int_no;
    
    interrupt_counts[int_no]++;
    
    if (int_no < 32) {
        // 双重错误特殊处理
        if (int_no == 8) {
            double_fault_handler(regs);
            return;  // 永不返回
        }
        
        // 其他异常的通用处理
        __asm__ volatile("cli");
        print_string("\n========================================\n");
        print_string("\nSYSTEAM IS DIED. \n");
        print_string("\n========================================\n");
        print_string("[EXCEPTION] ");
        print_string(exception_messages[int_no]);
        print_string("\n");
        print_string("INT: ");
        print_dec(int_no);
        print_string("  ERR: ");
        print_hex((uint32_t)regs->err_code);
        print_string("\n");
        print_string("RIP: ");
        print_hex((uint32_t)regs->rip);
        print_string("\n");
        print_string("RSP: ");
        print_hex((uint32_t)regs->rsp);
        print_string("\n");
        print_string("========================================\n");
        
        print_string("System halted.\n");
        while(1) {
            __asm__ volatile("hlt");
        }
    }
}

// 静态标志，防止中断重入
static volatile int in_irq = 0;

// IRQ处理函数
void irq_handler(registers_t* regs) {
    uint32_t int_no = (uint32_t)regs->int_no;
    
    // 防止重入
    if (in_irq) {
        uint8_t irq = int_no - 32;
        pic_send_eoi(irq);
        return;
    }
    in_irq = 1;
    
    interrupt_counts[int_no]++;
    
    uint8_t irq = int_no - 32;
    
    // 先发送EOI，避免PIC阻塞
    pic_send_eoi(irq);
    
    // 检查优先级，决定是否执行
    if (!irq_should_execute(irq)) {
        in_irq = 0;
        return;
    }
    
    // 进入中断处理
    irq_enter(irq);
    
    switch (irq) {
        case 0:  // IRQ0 - Timer
            timer_irq_handler();
            break;
            
        case 1:  // IRQ1 - Keyboard
            keyboard_irq_handler();
            break;
            
        default:
            break;
    }
    
    // 退出中断处理
    irq_exit();
    
    in_irq = 0;
}

uint64_t get_interrupt_count(uint8_t int_no) {
    if (int_no < 256) {
        return interrupt_counts[int_no];
    }
    return 0;
}
