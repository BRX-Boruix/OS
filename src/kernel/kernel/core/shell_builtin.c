// Boruix OS Shell - 内置命令实现
// 实现shell的内置命令功能

#include "shell_builtin.h"
#include "shell_command.h"
#include "drivers/display.h"
#include "drivers/cmos.h"
#include "shell_system.h"

// === 内置命令实现 ===

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

void cmd_clear(int argc, char* argv[]) {
    (void)argc; (void)argv;
    clear_screen();
}

void cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) print_char(' ');
        print_string(argv[i]);
    }
    print_char('\n');
}

void cmd_time(int argc, char* argv[]) {
    (void)argc; (void)argv;
    print_string("Current time: ");
    print_current_time();
    print_char('\n');
}

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

void cmd_reboot(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    print_string("Rebooting system...\n");
    print_string("Goodbye!\n");
    
    // 等待一下让用户看到消息
    for (volatile int i = 0; i < 10000000; i++);
    
    // 重启系统
    reboot_system();
}
