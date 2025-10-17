// Boruix OS test命令实现
// 测试命令，用于调试

#include "kernel/shell.h"
#include "drivers/display.h"
#include "drivers/cmos.h"

// 测试命令
void cmd_test(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    print_string("Test Command\n");
    print_string("------------\n");
    
    // 测试print_hex
    print_string("Hex test: ");
    print_hex(0x12345678);
    print_string("\n");
    
    // 测试print_dec
    print_string("Dec test: ");
    print_dec(12345);
    print_string("\n");
    
    // 显示当前时间
    print_string("Current time: ");
    uint8_t sec = read_cmos(0x00);
    uint8_t min = read_cmos(0x02);
    uint8_t hour = read_cmos(0x04);
    
    print_dec(hour);
    print_char(':');
    if (min < 10) print_char('0');
    print_dec(min);
    print_char(':');
    if (sec < 10) print_char('0');
    print_dec(sec);
    print_string("\n");
}
