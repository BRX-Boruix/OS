// Boruix OS Shell - 简单的命令行界面
// 提供基本的系统交互功能

#include "kernel/shell.h"
#include "drivers/display.h"
#include "drivers/cmos.h"
#include "drivers/keyboard.h"

// 全局变量
static char input_buffer[SHELL_BUFFER_SIZE];
static int buffer_pos = 0;

// 命令表
static shell_command_t commands[] = {
    {"help", "Show available commands", cmd_help},
    {"clear", "Clear screen", cmd_clear},
    {"echo", "Echo text", cmd_echo},
    {"time", "Show current time", cmd_time},
    {"info", "Show system information", cmd_info},
    {"reboot", "Reboot system", cmd_reboot},
    {NULL, NULL, NULL}  // 结束标记
};

// 字符串比较函数
int shell_strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// 字符串长度函数
int shell_strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

// 字符串复制函数
void shell_strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

// 简单的字符串分割函数
char* shell_strtok(char* str, const char* delim) {
    static char* last = NULL;
    if (str) last = str;
    if (!last) return NULL;
    
    // 跳过分隔符
    while (*last && *last == *delim) last++;
    if (!*last) return NULL;
    
    char* start = last;
    // 找到下一个分隔符
    while (*last && *last != *delim) last++;
    if (*last) {
        *last = '\0';
        last++;
    }
    
    return start;
}

// 显示提示符
void shell_print_prompt(void) {
    print_string(SHELL_PROMPT);
}

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

// Shell主循环
void shell_main(void) {
    shell_init();
    shell_print_prompt();
    
    while (1) {
        // 轮询键盘输入
        unsigned char scancode = keyboard_read_scancode();
        if (scancode == 0) {
            continue; // 没有输入，继续轮询
        }
        
        // 转换为ASCII字符
        char c = keyboard_scancode_to_ascii(scancode);
        if (c == 0) {
            continue; // 不是可打印字符，继续轮询
        }
        
        if (c == '\n' || c == '\r') {
            // 回车键 - 处理命令
            print_char('\n');
            input_buffer[buffer_pos] = '\0';
            shell_process_command(input_buffer);
            buffer_pos = 0;
            shell_print_prompt();
        } else if (c == '\b') {
            // 退格键
            if (buffer_pos > 0) {
                buffer_pos--;
                print_char('\b');
                print_char(' ');
                print_char('\b');
            }
        } else if (c >= 32 && c <= 126) {
            // 可打印字符
            if (buffer_pos < SHELL_BUFFER_SIZE - 1) {
                input_buffer[buffer_pos++] = c;
                print_char(c);
            }
        }
    }
}

// Shell初始化
void shell_init(void) {
    buffer_pos = 0;
    
    // 初始化键盘驱动
    keyboard_init();
    
    print_string("Boruix Shell v1.0\n");
    print_string("==================\n\n");
}

// === 内置命令实现 ===

void cmd_help(int argc, char* argv[]) {
    (void)argc; (void)argv;  // 避免未使用参数警告
    
    print_string("Available commands:\n");
    print_string("==================\n");
    
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
    
    print_string("System Information:\n");
    print_string("==================\n");
#ifdef __x86_64__
    print_string("Architecture: x86_64 (64-bit)\n");
#else
    print_string("Architecture: i386 (32-bit)\n");
#endif
    print_string("OS: Boruix v1.0\n");
    print_string("Kernel: Boruix Kernel\n");
    print_string("Shell: Boruix Shell v1.0\n");
    print_string("Memory: Basic management (debug mode)\n");
    print_string("\n");
}

void cmd_reboot(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    print_string("Rebooting system...\n");
    print_string("Goodbye!\n");
    
    // 等待一下让用户看到消息
    for (volatile int i = 0; i < 10000000; i++);
    
    // 方法1: 通过8042键盘控制器重启
    reboot_system();
}

// 简单的端口输出函数
static void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a" (value), "Nd" (port));
}

// 简单的端口输入函数
static uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

// 系统重启实现
void reboot_system(void) {
    // 方法1: 通过8042键盘控制器重启（最常用）
    print_string("Attempting reboot via 8042 controller...\n");
    
    // 等待8042输入缓冲区清空
    int timeout = 100000;
    while ((inb(0x64) & 0x02) != 0 && timeout-- > 0);
    
    if (timeout > 0) {
        // 发送重启命令到8042
        outb(0x64, 0xFE);
        
        // 等待重启生效
        for (volatile int i = 0; i < 1000000; i++);
    }
    
    // 方法2: 通过Triple Fault（如果8042失败）
    print_string("8042 method failed, trying triple fault...\n");
    
#ifdef __x86_64__
    // 64位版本的triple fault
    __asm__ volatile (
        "cli\n"                    // 禁用中断
        "mov $0, %rax\n"           // 将0加载到RAX
        "lidt (%rax)\n"            // 加载无效的IDT
        "int $0\n"                 // 触发中断0（会导致triple fault）
    );
#else
    // 32位版本的triple fault
    __asm__ volatile (
        "cli\n"                    // 禁用中断
        "mov $0, %eax\n"           // 将0加载到EAX
        "lidt (%eax)\n"            // 加载无效的IDT
        "int $0\n"                 // 触发中断0（会导致triple fault）
    );
#endif
    
    // 方法3: 通过CPU重置（最后的尝试）
    print_string("Triple fault failed, trying CPU reset...\n");
    
#ifdef __x86_64__
    // 64位版本的CPU重置
    __asm__ volatile (
        "mov $0xFFFF, %rax\n"      // 加载段地址
        "mov $0x0000, %rbx\n"      // 加载偏移地址
        "push %rax\n"              // 压入段地址
        "push %rbx\n"              // 压入偏移地址
        "retf\n"                   // 远返回（跳转到重置向量）
    );
#else
    // 32位版本的CPU重置
    __asm__ volatile (
        "mov $0xFFFF, %eax\n"      // 加载段地址
        "mov $0x0000, %ebx\n"      // 加载偏移地址
        "push %eax\n"              // 压入段地址
        "push %ebx\n"              // 压入偏移地址
        "retf\n"                   // 远返回（跳转到重置向量）
    );
#endif
    
    // 如果所有方法都失败，进入无限循环
    print_string("All reboot methods failed. System halted.\n");
    while (1) {
        __asm__ volatile ("hlt");
    }
}
