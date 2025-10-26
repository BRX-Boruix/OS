// Boruix OS Shell进程实现
// 将Shell作为独立进程运行

#include "kernel/kthread.h"
#include "kernel/process.h"
#include "drivers/display.h"

// Shell主函数声明
extern void shell_main(void);

// Shell进程入口函数
static void shell_process_entry(void* arg) {
    (void)arg;
    
    extern void serial_puts(const char*);
    serial_puts("[SHELL] Shell process entry point reached\n");
    
    // 确保中断启用
    __asm__ volatile("sti");
    
    // 清屏
    extern void clear_screen(void);
    clear_screen();
    
    print_string("\n");
    print_string("========================================\n");
    print_string("Boruix OS Shell\n");
    print_string("========================================\n");
    print_string("Type 'help' for available commands\n\n");
    
    // 调用原有的Shell主函数
    shell_main();
    
    // Shell退出（正常情况下不应该到达这里）
    print_string("[SHELL_PROCESS] Shell exited unexpectedly\n");
    kthread_exit(0);
}

// 启动Shell进程
pid_t start_shell_process(void) {
    print_string("[INIT] Starting shell as process...\n");
    
    pid_t shell_pid = kthread_create(
        shell_process_entry,
        NULL,
        "shell",
        PRIORITY_NORMAL
    );
    
    if (shell_pid == INVALID_PID) {
        print_string("[INIT] Failed to start shell process\n");
        return INVALID_PID;
    }
    
    print_string("[INIT] Shell process started with PID: ");
    print_dec(shell_pid);
    print_string("\n");
    
    return shell_pid;
}

