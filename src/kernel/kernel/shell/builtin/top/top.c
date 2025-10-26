// top命令 - 显示系统资源使用情况
#include "kernel/shell.h"
#include "kernel/process.h"
#include "drivers/display.h"
#include "drivers/timer.h"
#include "../../../shell/utils/string.h"

void cmd_top(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    print_string("\n");
    print_string("========================================\n");
    print_string("Boruix OS - System Monitor\n");
    print_string("========================================\n\n");
    
    // 获取进程统计
    size_t total = process_get_count();
    size_t ready = scheduler_get_ready_queue_size();
    size_t blocked = scheduler_get_blocked_queue_size();
    
    print_string("Processes: ");
    print_dec((uint32_t)total);
    print_string(" total, ");
    print_dec((uint32_t)ready);
    print_string(" ready, ");
    print_dec((uint32_t)blocked);
    print_string(" blocked\n\n");
    
    // 获取调度器统计
    scheduler_stats_t stats;
    if (scheduler_get_stats(&stats) == 0) {
        print_string("Scheduler Statistics:\n");
        print_string("  Total schedules: ");
        print_dec((uint32_t)stats.total_schedules);
        print_string("\n");
        print_string("  Context switches: ");
        print_dec((uint32_t)stats.context_switches);
        print_string("\n");
        print_string("  Preemptions: ");
        print_dec((uint32_t)stats.preemptions);
        print_string("\n");
        print_string("  Idle time: ");
        print_dec((uint32_t)stats.idle_time);
        print_string("\n\n");
    }
    
    // 系统运行时间
    extern volatile uint32_t system_ticks;
    uint32_t seconds = system_ticks / TIMER_FREQ_HZ;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    
    print_string("Uptime: ");
    print_dec(hours);
    print_string("h ");
    print_dec(minutes % 60);
    print_string("m ");
    print_dec(seconds % 60);
    print_string("s\n");
    
    print_string("System ticks: ");
    print_dec(system_ticks);
    print_string("\n\n");
    
    print_string("========================================\n");
}

