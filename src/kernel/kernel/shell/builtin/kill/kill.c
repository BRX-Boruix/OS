// kill命令 - 终止进程
#include "kernel/shell.h"
#include "kernel/process.h"
#include "drivers/display.h"
#include "../../../shell/utils/string.h"

// 外部函数：检查是否为超级用户模式
extern int is_super_mode(void);

void cmd_kill(int argc, char** argv) {
    if (argc < 2) {
        print_string("Usage: kill <pid>\n");
        print_string("Terminate a process by PID\n");
        return;
    }
    
    // 解析PID
    pid_t pid = 0;
    for (int i = 0; argv[1][i] != '\0'; i++) {
        if (argv[1][i] >= '0' && argv[1][i] <= '9') {
            pid = pid * 10 + (argv[1][i] - '0');
        } else {
            print_string("Error: Invalid PID\n");
            return;
        }
    }
    
    // 检查权限：普通用户不能杀死系统进程
    if (pid <= 2 && !is_super_mode()) {
        print_string("Error: Permission denied\n");
        print_string("Cannot kill system processes (PID <= 2) in user mode\n");
        print_string("Use 'switch super' to enable super user mode\n");
        return;
    }
    
    // 超级用户模式下的严重警告
    if (pid <= 2 && is_super_mode()) {
        print_string("\n");
        print_string("========================================\n");
        print_string("         CRITICAL WARNING!\n");
        print_string("========================================\n");
        print_string("You are about to terminate ");
        
        const char* process_names[] = {
            "KERNEL (PID 0)",
            "IDLE (PID 1)", 
            "INIT (PID 2)"
        };
        
        if (pid <= 2) {
            print_string(process_names[pid]);
        }
        
        print_string("\n\n");
        print_string("This is a CRITICAL SYSTEM PROCESS!\n");
        print_string("Terminating it WILL CRASH THE SYSTEM!\n");
        print_string("\n");
        print_string("The system will panic immediately.\n");
        print_string("========================================\n\n");
    }
    
    // 终止进程
    int result = process_destroy(pid);
    
    if (result == 0) {
        print_string("Process ");
        print_dec(pid);
        print_string(" terminated\n");
        
        // 如果是系统进程，这里不会执行到（会panic）
    } else {
        print_string("Error: Failed to terminate process ");
        print_dec(pid);
        print_string("\n");
    }
}
