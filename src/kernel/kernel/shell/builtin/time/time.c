// Boruix OS Shell - time命令实现
// 显示当前时间

#include "time.h"
#include "drivers/display.h"
#include "drivers/cmos.h"

void cmd_time(int argc, char* argv[]) {
    (void)argc; (void)argv;
    print_string("Current time: ");
    print_current_time();
    print_char('\n');
}
