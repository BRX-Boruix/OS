// prio命令 - 设置进程优先级
#include "kernel/shell.h"
#include "kernel/process.h"
#include "drivers/display.h"
#include "../../../shell/utils/string.h"

void cmd_prio(int argc, char** argv) {
    if (argc < 3) {
        print_string("Usage: prio <pid> <priority>\n");
        print_string("Set process priority (0=Idle, 1=Low, 2=Normal, 3=High, 4=Realtime)\n");
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
    
    // 解析优先级
    uint8_t priority = 0;
    for (int i = 0; argv[2][i] != '\0'; i++) {
        if (argv[2][i] >= '0' && argv[2][i] <= '9') {
            priority = priority * 10 + (argv[2][i] - '0');
        } else {
            print_string("Error: Invalid priority\n");
            return;
        }
    }
    
    if (priority > 4) {
        print_string("Error: Priority must be 0-4\n");
        return;
    }
    
    // 设置优先级
    int result = process_set_priority(pid, priority);
    
    if (result == 0) {
        print_string("Process ");
        print_dec(pid);
        print_string(" priority set to ");
        print_dec(priority);
        
        const char* prio_names[] = {"Idle", "Low", "Normal", "High", "Realtime"};
        print_string(" (");
        print_string(prio_names[priority]);
        print_string(")\n");
    } else {
        print_string("Error: Failed to set priority for process ");
        print_dec(pid);
        print_string("\n");
    }
}

