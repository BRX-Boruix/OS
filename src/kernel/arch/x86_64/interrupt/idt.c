// Boruix OS x86_64架构IDT实现

#include "kernel/interrupt.h"
#include "arch/x86_64.h"
#include "drivers/display.h"

static idt_entry_t idt[IDT_SIZE];
static idt_ptr_t idt_ptr;

extern void isr0(), isr1(), isr2(), isr3(), isr4(), isr5(), isr6(), isr7();
extern void isr8(), isr9(), isr10(), isr11(), isr12(), isr13(), isr14(), isr15();
extern void isr16(), isr17(), isr18(), isr19(), isr20(), isr21(), isr22(), isr23();
extern void isr24(), isr25(), isr26(), isr27(), isr28(), isr29(), isr30(), isr31();

extern void irq0(), irq1(), irq2(), irq3(), irq4(), irq5(), irq6(), irq7();
extern void irq8(), irq9(), irq10(), irq11(), irq12(), irq13(), irq14(), irq15();

extern void idt_load(uint64_t);

void idt_set_gate(uint8_t num, uint64_t handler) {
    idt[num].offset_low = (uint16_t)(handler & 0xFFFF);
    idt[num].offset_mid = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[num].offset_high = (uint32_t)((handler >> 32) & 0xFFFFFFFF);
    idt[num].selector = 0x08;  // 使用自己的GDT：内核代码段（第1项）
    idt[num].ist = 0;
    idt[num].type = IDT_INTERRUPT_GATE;
    idt[num].reserved = 0;
}

// 设置IDT门（带IST支持）
void idt_set_gate_with_ist(uint8_t num, uint64_t handler, uint8_t ist) {
    idt[num].offset_low = (uint16_t)(handler & 0xFFFF);
    idt[num].offset_mid = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[num].offset_high = (uint32_t)((handler >> 32) & 0xFFFFFFFF);
    idt[num].selector = 0x08;  // 使用自己的GDT：内核代码段（第1项）
    idt[num].ist = ist;  // 使用IST（1-7）
    idt[num].type = IDT_INTERRUPT_GATE;
    idt[num].reserved = 0;
}

void idt_init(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;
    
    for (int i = 0; i < IDT_SIZE; i++) {
        idt[i].offset_low = 0;
        idt[i].offset_mid = 0;
        idt[i].offset_high = 0;
        idt[i].selector = 0;
        idt[i].ist = 0;
        idt[i].type = 0;
        idt[i].reserved = 0;
    }
    
    idt_set_gate(0, (uint64_t)isr0);
    idt_set_gate(1, (uint64_t)isr1);
    idt_set_gate(2, (uint64_t)isr2);
    idt_set_gate(3, (uint64_t)isr3);
    idt_set_gate(4, (uint64_t)isr4);
    idt_set_gate(5, (uint64_t)isr5);
    idt_set_gate(6, (uint64_t)isr6);
    idt_set_gate(7, (uint64_t)isr7);
    idt_set_gate_with_ist(8, (uint64_t)isr8, 1);  // 双重错误使用IST1
    idt_set_gate(9, (uint64_t)isr9);
    idt_set_gate(10, (uint64_t)isr10);
    idt_set_gate(11, (uint64_t)isr11);
    idt_set_gate(12, (uint64_t)isr12);
    idt_set_gate(13, (uint64_t)isr13);
    idt_set_gate(14, (uint64_t)isr14);
    idt_set_gate(15, (uint64_t)isr15);
    idt_set_gate(16, (uint64_t)isr16);
    idt_set_gate(17, (uint64_t)isr17);
    idt_set_gate(18, (uint64_t)isr18);
    idt_set_gate(19, (uint64_t)isr19);
    idt_set_gate(20, (uint64_t)isr20);
    idt_set_gate(21, (uint64_t)isr21);
    idt_set_gate(22, (uint64_t)isr22);
    idt_set_gate(23, (uint64_t)isr23);
    idt_set_gate(24, (uint64_t)isr24);
    idt_set_gate(25, (uint64_t)isr25);
    idt_set_gate(26, (uint64_t)isr26);
    idt_set_gate(27, (uint64_t)isr27);
    idt_set_gate(28, (uint64_t)isr28);
    idt_set_gate(29, (uint64_t)isr29);
    idt_set_gate(30, (uint64_t)isr30);
    idt_set_gate(31, (uint64_t)isr31);
    
    idt_set_gate(32, (uint64_t)irq0);
    idt_set_gate(33, (uint64_t)irq1);
    idt_set_gate(34, (uint64_t)irq2);
    idt_set_gate(35, (uint64_t)irq3);
    idt_set_gate(36, (uint64_t)irq4);
    idt_set_gate(37, (uint64_t)irq5);
    idt_set_gate(38, (uint64_t)irq6);
    idt_set_gate(39, (uint64_t)irq7);
    idt_set_gate(40, (uint64_t)irq8);
    idt_set_gate(41, (uint64_t)irq9);
    idt_set_gate(42, (uint64_t)irq10);
    idt_set_gate(43, (uint64_t)irq11);
    idt_set_gate(44, (uint64_t)irq12);
    idt_set_gate(45, (uint64_t)irq13);
    idt_set_gate(46, (uint64_t)irq14);
    idt_set_gate(47, (uint64_t)irq15);
    
    idt_load((uint64_t)&idt_ptr);
    
    print_string("[IDT] Interrupt Descriptor Table initialized (x86_64)\n");
}
