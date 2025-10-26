// Boruix OS 系统监控进程
// 定期显示系统状态信息

#include "kernel/kthread.h"
#include "kernel/process.h"
#include "drivers/display.h"
#include "drivers/timer.h"

// 系统监控进程入口函数
static void sysmon_process_entry(void* arg) {
    (void)arg;
    
    extern void serial_puts(const char*);
    serial_puts("[SYSMON] System monitor process started\n");
    
    while (1) {
        // 休眠 30 秒（30000 毫秒）
        kthread_sleep(30000);
        
        // 获取进程数量
        size_t process_count = process_get_count();
        size_t ready_count = scheduler_get_ready_queue_size();
        size_t blocked_count = scheduler_get_blocked_queue_size();
        
        // 获取调度器统计（只输出到串口，不显示在屏幕上）
        scheduler_stats_t stats;
        if (scheduler_get_stats(&stats) == 0) {
            extern void serial_puts(const char*);
            extern void serial_put_dec(uint32_t);
            
            serial_puts("\n[SYSMON] === System Status ===\n");
            serial_puts("[SYSMON] Processes: ");
            serial_put_dec((uint32_t)process_count);
            serial_puts(" (Ready: ");
            serial_put_dec((uint32_t)ready_count);
            serial_puts(", Blocked: ");
            serial_put_dec((uint32_t)blocked_count);
            serial_puts(")\n");
            
            serial_puts("[SYSMON] Context switches: ");
            serial_put_dec((uint32_t)stats.context_switches);
            serial_puts(", Preemptions: ");
            serial_put_dec((uint32_t)stats.preemptions);
            serial_puts("\n");
            
            serial_puts("[SYSMON] System ticks: ");
            extern volatile uint32_t system_ticks;
            serial_put_dec(system_ticks);
            serial_puts("\n");
            serial_puts("[SYSMON] =====================\n\n");
        }
    }
}

// 启动系统监控进程
pid_t start_sysmon_process(void) {
    print_string("[INIT] Starting system monitor process...\n");
    
    pid_t sysmon_pid = kthread_create(
        sysmon_process_entry,
        NULL,
        "sysmon",
        PRIORITY_LOW  // 低优先级，不影响其他进程
    );
    
    if (sysmon_pid == INVALID_PID) {
        print_string("[INIT] Failed to start system monitor\n");
        return INVALID_PID;
    }
    
    print_string("[INIT] System monitor started with PID: ");
    print_dec(sysmon_pid);
    print_string("\n");
    
    return sysmon_pid;
}

