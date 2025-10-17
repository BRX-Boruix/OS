// Boruix OS irqinfo命令 - 显示IRQ配置信息

#include "kernel/shell.h"
#include "kernel/interrupt.h"
#include "drivers/display.h"

void cmd_irqinfo(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    print_string("IRQ Configuration Information\n");
    print_string("========================================\n\n");
    
    // 显示中断状态
    print_string("Interrupt Status: ");
    if (interrupts_enabled()) {
        print_string("ENABLED\n");
    } else {
        print_string("DISABLED\n");
    }
    print_string("\n");
    
    // IRQ映射表
    print_string("IRQ Mapping (PIC 8259A):\n");
    print_string("------------------------\n");
    print_string("IRQ  INT  Device\n");
    print_string("---  ---  -----------------\n");
    
    const char* irq_names[] = {
        "Timer (PIT)",
        "Keyboard (PS/2)",
        "Cascade (PIC2)",
        "COM2",
        "COM1",
        "LPT2",
        "Floppy Disk",
        "LPT1",
        "RTC",
        "Available",
        "Available",
        "Available",
        "PS/2 Mouse",
        "FPU",
        "Primary ATA",
        "Secondary ATA"
    };
    
    for (int i = 0; i < 16; i++) {
        print_string("IRQ");
        if (i < 10) print_char(' ');
        print_dec(i);
        print_string("  ");
        print_dec(32 + i);
        print_string("  ");
        print_string(irq_names[i]);
        print_string("\n");
    }
    
    print_string("\n");
    print_string("PIC Base Vectors:\n");
    print_string("  Master PIC: INT 32-39 (IRQ 0-7)\n");
    print_string("  Slave PIC:  INT 40-47 (IRQ 8-15)\n");
}
