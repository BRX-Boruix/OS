// Boruix OS inton命令 - 启用中断

#include "kernel/shell.h"
#include "drivers/display.h"
#include "kernel/interrupt.h"

void cmd_inton(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    if (interrupts_enabled()) {
        print_string("Interrupts are already enabled.\n");
        return;
    }
    
    print_string("Enabling interrupts...\n");
    interrupts_enable();
    
    if (interrupts_enabled()) {
        print_string("Interrupts enabled successfully!\n");
        print_string("Timer interrupts should now be working.\n");
        print_string("Use 'irqstat' to check interrupt statistics.\n");
    } else {
        print_string("Failed to enable interrupts!\n");
    }
}
