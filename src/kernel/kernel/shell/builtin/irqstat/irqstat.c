// Boruix OS irqstat命令 - 显示中断统计信息

#include "kernel/shell.h"
#include "drivers/display.h"
#include "drivers/timer.h"

// 外部函数
extern uint32_t get_interrupt_count(uint8_t int_no);

void cmd_irqstat(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    print_string("Interrupt Statistics\n");
    print_string("========================================\n\n");
    
    // 显示系统运行时间
    uint32_t total_seconds = timer_get_seconds();
    uint32_t hours = total_seconds / 3600;
    uint32_t minutes = (total_seconds % 3600) / 60;
    uint32_t seconds = total_seconds % 60;
    
    print_string("System Uptime: ");
    print_dec(hours);
    print_char(':');
    if (minutes < 10) print_char('0');
    print_dec(minutes);
    print_char(':');
    if (seconds < 10) print_char('0');
    print_dec(seconds);
    print_string(" (");
    print_dec(system_ticks);
    print_string(" ticks)\n\n");
    
    // 显示中断计数
    print_string("IRQ Statistics:\n");
    print_string("IRQ  Count      Rate/s  Description\n");
    print_string("---  ---------  ------  ---------------------\n");
    
    const char* irq_names[] = {
        "Timer",
        "Keyboard",
        "Cascade",
        "COM2",
        "COM1",
        "LPT2",
        "Floppy",
        "LPT1",
        "RTC",
        "Free",
        "Free",
        "Free",
        "Mouse",
        "FPU",
        "ATA1",
        "ATA2"
    };
    
    int active_irqs = 0;
    for (int i = 0; i < 16; i++) {
        uint32_t count = get_interrupt_count(32 + i);
        if (count > 0) {
            active_irqs++;
            
            // IRQ号
            print_string("IRQ");
            if (i < 10) print_char(' ');
            print_dec(i);
            print_string("  ");
            
            // 计数
            print_dec(count);
            // 补齐空格
            int spaces = 9;
            uint32_t temp = count;
            while (temp > 0) {
                spaces--;
                temp /= 10;
            }
            for (int j = 0; j < spaces; j++) print_char(' ');
            print_string("  ");
            
            // 频率（每秒触发次数）
            if (total_seconds > 0) {
                uint32_t rate = count / total_seconds;
                print_dec(rate);
                print_string("      ");
            } else {
                print_string("N/A    ");
            }
            
            // 描述
            print_string(irq_names[i]);
            print_string("\n");
        }
    }
    
    if (active_irqs == 0) {
        print_string("No IRQ activity detected.\n");
    }
    
    print_string("\n");
    print_string("Tip: Use 'irqinfo' to see IRQ configuration\n");
}
