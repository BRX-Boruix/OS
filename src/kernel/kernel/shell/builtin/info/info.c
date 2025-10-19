// Boruix OS Shell - info命令实现
// 显示系统信息

#include "info.h"
#include "drivers/display.h"

void cmd_info(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    print_string("BORUIX SYSTEM INFORMATION\n");
    print_string("==================\n");
    print_string("Architecture: x86_64 (64-bit)\n");
    print_string("\n\n");
}
