// Boruix OS nice命令实现
// 修改进程优先级

#include "drivers/display.h"
#include "kernel/process.h"
#include "kernel/types.h"

// 字符串转整数
static int str_to_int(const char* str) {
    int result = 0;
    int sign = 1;
    
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

void cmd_nice(int argc, char** argv) {
    if (argc < 3) {
        print_string("Usage: nice <pid> <priority>\n");
        print_string("Priority: 0=Realtime, 1=High, 2=Normal, 3=Low, 4=Idle\n");
        return;
    }
    
    pid_t pid = (pid_t)str_to_int(argv[1]);
    int priority = str_to_int(argv[2]);
    
    if (priority < 0 || priority > 4) {
        print_string("Invalid priority. Must be 0-4\n");
        return;
    }
    
    if (process_set_priority(pid, (process_priority_t)priority) == 0) {
        print_string("Process ");
        print_dec(pid);
        print_string(" priority set to ");
        print_dec(priority);
        print_char('\n');
    } else {
        print_string("Failed to set priority for process ");
        print_dec(pid);
        print_char('\n');
    }
}

