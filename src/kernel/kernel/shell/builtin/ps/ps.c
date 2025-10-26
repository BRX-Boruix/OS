// Boruix OS ps命令实现
// 显示进程列表

#include "drivers/display.h"
#include "kernel/process.h"
#include "kernel/types.h"

// 进程状态名称
static const char* state_names[] = {
    "Created",
    "Ready",
    "Running",
    "Blocked",
    "Zombie",
    "Terminated"
};

// 优先级名称
static const char* priority_names[] = {
    "Realtime",
    "High",
    "Normal",
    "Low",
    "Idle"
};

void cmd_ps(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    print_string("Process List:\n");
    print_string("================================================================================\n");
    print_string("PID   PPID  STATE      PRIORITY   NAME                CPU TIME\n");
    print_string("--------------------------------------------------------------------------------\n");
    
    // 获取进程数量
    size_t process_count = process_get_count();
    
    if (process_count == 0) {
        print_string("No processes running\n");
        return;
    }
    
    // 遍历所有可能的PID
    for (pid_t pid = 1; pid < 256; pid++) {
        process_info_t info;
        
        if (process_get_info(pid, &info) == 0) {
            // PID
            print_dec(info.pid);
            print_string("     ");
            
            // PPID
            print_dec(info.parent_pid);
            print_string("     ");
            
            // STATE
            if (info.state < 6) {
                print_string(state_names[info.state]);
                // 对齐
                size_t len = 0;
                const char* s = state_names[info.state];
                while (s[len]) len++;
                for (size_t i = len; i < 10; i++) {
                    print_char(' ');
                }
            } else {
                print_string("Unknown   ");
            }
            print_string(" ");
            
            // PRIORITY
            if (info.priority < 5) {
                print_string(priority_names[info.priority]);
                // 对齐
                size_t len = 0;
                const char* s = priority_names[info.priority];
                while (s[len]) len++;
                for (size_t i = len; i < 10; i++) {
                    print_char(' ');
                }
            } else {
                print_string("Unknown   ");
            }
            print_string(" ");
            
            // NAME
            print_string(info.name);
            // 对齐
            size_t name_len = 0;
            while (info.name[name_len] && name_len < 32) name_len++;
            for (size_t i = name_len; i < 20; i++) {
                print_char(' ');
            }
            
            // CPU TIME
            print_dec((uint32_t)(info.cpu_time / 1000000));  // 转换为毫秒
            print_string(" ms");
            
            print_char('\n');
        }
    }
    
    print_string("--------------------------------------------------------------------------------\n");
    print_string("Total processes: ");
    print_dec((uint32_t)process_count);
    print_char('\n');
    
    // 显示队列信息
    size_t ready_count = scheduler_get_ready_queue_size();
    size_t blocked_count = scheduler_get_blocked_queue_size();
    
    print_string("Ready queue: ");
    print_dec((uint32_t)ready_count);
    print_string(", Blocked queue: ");
    print_dec((uint32_t)blocked_count);
    print_char('\n');
}
