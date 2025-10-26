// Boruix OS Init进程实现
// 系统初始化进程（PID 1），负责启动其他系统进程

#include "kernel/kthread.h"
#include "kernel/process.h"
#include "kernel/interrupt.h"
#include "drivers/display.h"

// Shell进程启动函数声明
extern pid_t start_shell_process(void);

// 系统监控进程启动函数声明
extern pid_t start_sysmon_process(void);

// Init进程入口函数
static void init_process_entry(void* arg) {
    (void)arg;
    
    extern void serial_puts(const char*);
    serial_puts("[INIT] Init process entry point reached!\n");
    
    // 确保中断已启用
    __asm__ volatile("sti");
    serial_puts("[INIT] Interrupts enabled\n");
    
    // 清屏，避免显示乱码
    extern void clear_screen(void);
    clear_screen();
    
    print_string("\n");
    print_string("========================================\n");
    print_string("[INIT] Init process (PID 1) started\n");
    print_string("========================================\n");
    
    serial_puts("[INIT] About to start system monitor...\n");
    
    // 启动系统监控进程
    pid_t sysmon_pid = start_sysmon_process();
    serial_puts("[INIT] start_sysmon_process() returned\n");
    
    if (sysmon_pid == INVALID_PID) {
        print_string("[INIT] Warning: Failed to start system monitor\n");
    } else {
        print_string("[INIT] System monitor started successfully\n");
    }
    
    // 启动Shell进程
    serial_puts("[INIT] About to start shell...\n");
    pid_t shell_pid = start_shell_process();
    serial_puts("[INIT] start_shell_process() returned\n");
    
    if (shell_pid == INVALID_PID) {
        print_string("[INIT] Critical: Failed to start shell\n");
        print_string("[INIT] System halted\n");
        while (1) {
            __asm__ volatile("hlt");
        }
    }
    
    print_string("[INIT] System initialization complete\n");
    print_string("[INIT] Running processes:\n");
    print_string("[INIT]   - PID 0: kernel (kernel itself)\n");
    print_string("[INIT]   - PID 1: idle (system idle process)\n");
    print_string("[INIT]   - PID 2: init (system init process)\n");
    if (sysmon_pid != INVALID_PID) {
        print_string("[INIT]   - PID 3: sysmon (system monitor)\n");
    }
    print_string("[INIT]   - PID 4: shell (user shell)\n");
    print_string("========================================\n\n");
    
    // 抢占式调度已经在内核中启用
    print_string("[INIT] Preemptive scheduling is active\n\n");
    
    serial_puts("[INIT] Init process yielding to shell...\n");
    
    // 多次让出CPU，确保shell进程有机会运行
    for (int i = 0; i < 10; i++) {
        kthread_yield();
    }
    
    serial_puts("[INIT] Init process yielded 10 times\n");
    
    // Init进程进入空闲循环，监控系统状态
    while (1) {
        // 使用hlt等待中断（节省CPU）
        // 让调度器自动抢占，不需要每次都yield
        __asm__ volatile("hlt");
        
        // 简单的心跳（每隔一段时间）
        static uint32_t heartbeat_counter = 0;
        heartbeat_counter++;
        
        if (heartbeat_counter >= 100000) {
            extern void serial_puts(const char*);
            serial_puts("[INIT] Heartbeat\n");
            heartbeat_counter = 0;
        }
    }
}

// 启动Init进程
pid_t start_init_process(void) {
    print_string("[KERNEL] Starting init process...\n");
    
    // 创建Init进程（高优先级）
    pid_t init_pid = kthread_create(
        init_process_entry,
        NULL,
        "init",
        PRIORITY_HIGH
    );
    
    if (init_pid == INVALID_PID) {
        print_string("[KERNEL] Failed to start init process\n");
        return INVALID_PID;
    }
    
    // Init进程应该是PID 1（在idle之后）
    print_string("[KERNEL] Init process started with PID: ");
    print_dec(init_pid);
    print_string("\n");
    
    return init_pid;
}

