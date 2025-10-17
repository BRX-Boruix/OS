// Boruix OS Shell - info命令实现
// 显示系统信息

#include "info.h"
#include "drivers/display.h"

void cmd_info(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    print_string("BORUIX SYSTEM INFORMATION\n");
    print_string("==================\n");
#ifdef __x86_64__
    print_string("Architecture: x86_64 (64-bit)\n");
#else
    print_string("Architecture: i386 (32-bit)\n");
#endif
    print_string("\n\n");
}
