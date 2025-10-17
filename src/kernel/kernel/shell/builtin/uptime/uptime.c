// Boruix OS uptime命令 - 显示系统运行时间

#include "kernel/shell.h"
#include "drivers/display.h"
#include "drivers/timer.h"

void cmd_uptime(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    uint32_t total_seconds = timer_get_seconds();
    uint32_t hours = total_seconds / 3600;
    uint32_t minutes = (total_seconds % 3600) / 60;
    uint32_t seconds = total_seconds % 60;
    
    print_string("System uptime: ");
    print_dec(hours);
    print_string(" hours, ");
    print_dec(minutes);
    print_string(" minutes, ");
    print_dec(seconds);
    print_string(" seconds\n");
    
    print_string("Total ticks: ");
    print_dec(system_ticks);
    print_string(" (");
    print_dec(TIMER_FREQ_HZ);
    print_string(" Hz)\n");
}
