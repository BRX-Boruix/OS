// Boruix OS intoff命令 - 禁用中断

#include "kernel/shell.h"
#include "drivers/display.h"
#include "kernel/interrupt.h"

void cmd_intoff(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    if (!interrupts_enabled()) {
        print_string("Interrupts are already disabled.\n");
        return;
    }
    
    print_string("Disabling interrupts...\n");
    interrupts_disable();
    
    if (!interrupts_enabled()) {
        print_string("Interrupts disabled successfully!\n");
    } else {
        print_string("Failed to disable interrupts!\n");
    }
}
