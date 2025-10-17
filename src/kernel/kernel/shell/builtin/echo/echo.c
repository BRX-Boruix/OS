// Boruix OS Shell - echo命令实现
// 回显文本功能

#include "echo.h"
#include "drivers/display.h"

void cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) print_char(' ');
        print_string(argv[i]);
    }
    print_char('\n');
}
