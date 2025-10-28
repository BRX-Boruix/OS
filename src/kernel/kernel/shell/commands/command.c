// Boruix OS Shell - 命令处理实现
// 处理命令解析和执行

#include "command.h"
#include "../builtin/builtin.h"
#include "../builtin/keytest/keytest.h"
#include "../builtin/lspci/lspci.h"
#include "kernel/shell.h"
#include "drivers/display.h"

// 测试命令声明（仅在 ENABLE_TEST_COMMANDS 定义时编译）
#ifdef ENABLE_TEST_COMMANDS
void cmd_memtest(int argc, char* argv[]);
void cmd_dftest(int argc, char* argv[]);
void cmd_pgtest(int argc, char* argv[]);
void cmd_vmmtest(int argc, char* argv[]);
void cmd_heaptest(int argc, char* argv[]);
void cmd_memprottest(int argc, char* argv[]);
#endif

// 命令表
static shell_command_t commands[] = {
    {"help", "Show available commands", cmd_help},
    {"clear", "Clear screen", cmd_clear},
    {"cls", "Clear screen(alias for clear)", cmd_clear},
    {"echo", "Echo text", cmd_echo},
    {"time", "Show current time", cmd_time},
    {"info", "Show system information", cmd_info},
    {"uptime", "Show system uptime", cmd_uptime},
    {"irqstat", "Show interrupt statistics", cmd_irqstat},
    {"irqinfo", "Show IRQ configuration", cmd_irqinfo},
    {"irqprio", "Manage IRQ priorities", cmd_irqprio},
    {"irqtest", "Test IRQ priority system", cmd_irqtest},
    {"reboot", "Reboot system", cmd_reboot},
    {"great", "Let the great Yang Borui give you the answer.", cmd_great},
    {"license", "Show license information for projects", cmd_license},
    {"keytest", "Test keyboard input and scancodes", cmd_keytest},
    {"lspci", "List all PCI devices", cmd_lspci},
#ifdef ENABLE_TEST_COMMANDS
    {"crash", "Manually trigger system crash", cmd_crash},
    {"dftest", "Test double fault handler", cmd_dftest},
    {"pgtest", "Test page table mapping and translation", cmd_pgtest},
    {"vmmtest", "Test virtual memory manager", cmd_vmmtest},
    {"heaptest", "Test heap allocator (kmalloc/kfree)", cmd_heaptest},
    {"memprottest", "Test memory protection mechanism", cmd_memprottest},
    {"memtest", "Test TTY memory management and page tables", cmd_memtest},
    {"test", "Test command", cmd_test},
#endif
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
