// Boruix OS schedstat命令实现
// 显示调度器统计信息

#include "drivers/display.h"
#include "kernel/process.h"
#include "kernel/types.h"

void cmd_schedstat(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    scheduler_stats_t stats;
    
    if (scheduler_get_stats(&stats) != 0) {
        print_string("Failed to get scheduler statistics\n");
        return;
    }
    
    print_string("Scheduler Statistics:\n");
    print_string("================================================================================\n");
    
    print_string("Total schedules:     ");
    print_dec((uint32_t)stats.total_schedules);
    print_char('\n');
    
    print_string("Context switches:    ");
    print_dec((uint32_t)stats.context_switches);
    print_char('\n');
    
    print_string("Preemptions:         ");
    print_dec((uint32_t)stats.preemptions);
    print_char('\n');
    
    print_string("Idle time:           ");
    print_dec((uint32_t)(stats.idle_time / 1000000));  // 转换为毫秒
    print_string(" ms\n");
    
    print_string("\nPriority Schedules:\n");
    print_string("  Realtime:          ");
    print_dec((uint32_t)stats.priority_schedules[0]);
    print_char('\n');
    
    print_string("  High:              ");
    print_dec((uint32_t)stats.priority_schedules[1]);
    print_char('\n');
    
    print_string("  Normal:            ");
    print_dec((uint32_t)stats.priority_schedules[2]);
    print_char('\n');
    
    print_string("  Low:               ");
    print_dec((uint32_t)stats.priority_schedules[3]);
    print_char('\n');
    
    print_string("  Idle:              ");
    print_dec((uint32_t)stats.priority_schedules[4]);
    print_char('\n');
    
    print_string("================================================================================\n");
}

