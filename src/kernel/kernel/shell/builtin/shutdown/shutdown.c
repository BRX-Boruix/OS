// Boruix OS Shell - shutdown命令实现
// 通用系统关机功能

#include "shutdown.h"
#include "drivers/display.h"
#include "../../utils/system.h"

void cmd_shutdown(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    for (volatile int i = 0; i < 5000000; i++);
    
    print_string("\nInitiating power-down sequence...\n");
    print_string("\n");
    
    // 调用系统关机
    shutdown_system();
} 