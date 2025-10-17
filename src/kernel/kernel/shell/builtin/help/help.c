// Boruix OS Shell - help命令实现
// 显示可用命令列表

#include "help.h"
#include "../../commands/command.h"
#include "drivers/display.h"

void cmd_help(int argc, char* argv[]) {
    (void)argc; (void)argv;  // 避免未使用参数警告
    
    print_string("BORUIX SHELL COMMANDS\n");
    print_string("==================\n");
    
    shell_command_t* commands = shell_get_commands();
    for (int i = 0; commands[i].name; i++) {
        print_string("  ");
        print_string(commands[i].name);
        print_string(" - ");
        print_string(commands[i].description);
        print_string("\n");
    }
    print_string("\n");
}
