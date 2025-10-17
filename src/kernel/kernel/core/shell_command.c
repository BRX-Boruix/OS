// Boruix OS Shell - 命令处理实现
// 处理命令解析和执行

#include "shell_command.h"
#include "shell_builtin.h"
#include "kernel/shell.h"
#include "drivers/display.h"

// 命令表
static shell_command_t commands[] = {
    {"help", "Show available commands", cmd_help},
    {"clear", "Clear screen", cmd_clear},
    {"cls", "Clear screen(alias for clear)", cmd_clear},
    {"echo", "Echo text", cmd_echo},
    {"time", "Show current time", cmd_time},
    {"info", "Show system information", cmd_info},
    {"reboot", "Reboot system", cmd_reboot},
    {NULL, NULL, NULL}  // 结束标记
};

// 处理命令
void shell_process_command(const char* input) {
    if (shell_strlen(input) == 0) return;
    
    // 复制输入到临时缓冲区
    char temp_buffer[SHELL_BUFFER_SIZE];
    shell_strcpy(temp_buffer, input);
    
    // 解析命令和参数
    char* argv[SHELL_MAX_ARGS];
    int argc = 0;
    
    char* token = shell_strtok(temp_buffer, " ");
    while (token && argc < SHELL_MAX_ARGS - 1) {
        argv[argc++] = token;
        token = shell_strtok(NULL, " ");
    }
    argv[argc] = NULL;
    
    if (argc == 0) return;
    
    // 查找并执行命令
    for (int i = 0; commands[i].name; i++) {
        if (shell_strcmp(argv[0], commands[i].name) == 0) {
            commands[i].function(argc, argv);
            return;
        }
    }
    
    // 命令未找到
    print_string("Command not found: ");
    print_string(argv[0]);
    print_string("\nType 'help' for available commands.\n");
}

// 获取命令表
shell_command_t* shell_get_commands(void) {
    return commands;
}
