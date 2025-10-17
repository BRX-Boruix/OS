// Boruix OS Shell - reboot命令实现
// 系统重启功能

#include "reboot.h"
#include "drivers/display.h"
#include "../../utils/system.h"

void cmd_reboot(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    print_string("Rebooting system...\n");
    print_string("Goodbye!\n");
    
    // 等待一下让用户看到消息
    for (volatile int i = 0; i < 10000000; i++);
    
    // 重启系统
    reboot_system();
}
