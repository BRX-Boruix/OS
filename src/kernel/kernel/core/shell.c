// Boruix OS Shell - 简单的命令行界面
// 提供基本的系统交互功能

#include "kernel/shell.h"
#include "drivers/display.h"
#include "drivers/cmos.h"
#include "drivers/keyboard.h"

// 全局变量
static char input_buffer[SHELL_BUFFER_SIZE];
static int buffer_pos = 0;
static int cursor_pos = 0;  // 光标在输入缓冲区中的位置

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

// 显示控制字符（如^C, ^V等）
void shell_display_control_char(uint8_t key) {
    print_char('^');
    // 将扫描码转换为控制字符显示
    switch (key) {
        case 0x2E: // C键
            print_char('C');
            break;
        case 0x2F: // V键
            print_char('V');
            break;
        case 0x26: // L键
            print_char('L');
            break;
        case 0x16: // U键
            print_char('U');
            break;
        case 0x25: // K键
            print_char('K');
            break;
        case 0x1E: // A键
            print_char('A');
            break;
        case 0x12: // E键
            print_char('E');
            break;
        case 0x11: // W键
            print_char('W');
            break;
        case 0x20: // D键
            print_char('D');
            break;
        case 0x1C: // 回车键
            print_char('M');
            break;
        case 0x0E: // 退格键
            print_char('H');
            break;
        case 0x0F: // Tab键
            print_char('I');
            break;
        case 0x01: // Esc键
            print_char('[');
            break;
        default:
            // 对于其他键，显示十六进制
            print_char('0');
            print_char('x');
            if (key < 16) print_char('0');
            // 简单的十六进制显示
            if (key >= 16) {
                print_char('1');
                key -= 16;
            }
            if (key >= 10) {
                print_char('A' + (key - 10));
            } else {
                print_char('0' + key);
            }
            break;
    }
}

// 通用组合键处理函数
void shell_handle_combo_sequence(uint8_t* sequence, int length, uint8_t modifiers) {
    if (length == 0) return;
    
    // 检查是否是Ctrl组合键
    if (modifiers & 0x02) { // Ctrl键被按下
        if (length == 1) {
            uint8_t key = sequence[0];
            
            // 根据按键处理不同的Ctrl组合键
            switch (key) {
                case 0x2E: // Ctrl+C (C键)
                    shell_display_control_char(key);
                    print_char('\n');
                    buffer_pos = 0;
                    cursor_pos = 0;
                    shell_print_prompt();
                    break;
                    
                case 0x2F: // Ctrl+V (V键)
                    shell_display_control_char(key);
                    print_string(" - Paste (not implemented)\n");
                    shell_print_prompt();
                    break;
                    
                case 0x26: // Ctrl+L (L键)
                    shell_display_control_char(key);
                    print_char('\n');
                    clear_screen();
                    shell_print_prompt();
                    if (buffer_pos > 0) {
                        print_string(input_buffer);
                    }
                    break;
                    
                case 0x16: // Ctrl+U (U键)
                    shell_display_control_char(key);
                    print_string(" - Delete to beginning of line\n");
                    if (cursor_pos > 0) {
                        // 删除到行首
                        for (int i = 0; i < cursor_pos; i++) {
                            print_char('\b');
                        }
                        int chars_to_delete = cursor_pos;
                        for (int i = 0; i < chars_to_delete; i++) {
                            print_char(' ');
                        }
                        for (int i = 0; i < chars_to_delete; i++) {
                            print_char('\b');
                        }
                        for (int i = cursor_pos; i < buffer_pos; i++) {
                            input_buffer[i - cursor_pos] = input_buffer[i];
                            print_char(input_buffer[i]);
                        }
                        for (int i = buffer_pos; i > buffer_pos - cursor_pos; i--) {
                            print_char(' ');
                        }
                        for (int i = buffer_pos; i > buffer_pos - cursor_pos; i--) {
                            print_char('\b');
                        }
                        buffer_pos -= cursor_pos;
                        cursor_pos = 0;
                    }
                    shell_print_prompt();
                    break;
                    
                case 0x25: // Ctrl+K (K键)
                    shell_display_control_char(key);
                    print_string(" - Delete to end of line\n");
                    if (cursor_pos < buffer_pos) {
                        int chars_to_delete = buffer_pos - cursor_pos;
                        for (int i = 0; i < chars_to_delete; i++) {
                            print_char(' ');
                        }
                        for (int i = 0; i < chars_to_delete; i++) {
                            print_char('\b');
                        }
                        buffer_pos = cursor_pos;
                    }
                    shell_print_prompt();
                    break;
                    
                case 0x1E: // Ctrl+A (A键)
                    shell_display_control_char(key);
                    print_string(" - Move to beginning of line\n");
                    for (int i = 0; i < cursor_pos; i++) {
                        print_char('\b');
                    }
                    cursor_pos = 0;
                    break;
                    
                case 0x12: // Ctrl+E (E键)
                    shell_display_control_char(key);
                    print_string(" - Move to end of line\n");
                    for (int i = cursor_pos; i < buffer_pos; i++) {
                        print_char(input_buffer[i]);
                    }
                    cursor_pos = buffer_pos;
                    break;
                    
                case 0x11: // Ctrl+W (W键)
                    shell_display_control_char(key);
                    print_string(" - Delete previous word\n");
                    if (cursor_pos > 0) {
                        int word_start = cursor_pos;
                        while (word_start > 0 && input_buffer[word_start - 1] == ' ') {
                            word_start--;
                        }
                        while (word_start > 0 && input_buffer[word_start - 1] != ' ') {
                            word_start--;
                        }
                        
                        int chars_to_delete = cursor_pos - word_start;
                        for (int i = 0; i < chars_to_delete; i++) {
                            print_char('\b');
                        }
                        for (int i = 0; i < chars_to_delete; i++) {
                            print_char(' ');
                        }
                        for (int i = 0; i < chars_to_delete; i++) {
                            print_char('\b');
                        }
                        for (int i = cursor_pos; i < buffer_pos; i++) {
                            input_buffer[i - chars_to_delete] = input_buffer[i];
                            print_char(input_buffer[i]);
                        }
                        for (int i = buffer_pos; i > buffer_pos - chars_to_delete; i--) {
                            print_char(' ');
                        }
                        for (int i = buffer_pos; i > buffer_pos - chars_to_delete; i--) {
                            print_char('\b');
                        }
                        buffer_pos -= chars_to_delete;
                        cursor_pos = word_start;
                    }
                    break;
                    
                case 0x20: // Ctrl+D (D键)
                    shell_display_control_char(key);
                    print_string(" - EOF signal\n");
                    if (buffer_pos == 0) {
                        print_string("\nGoodbye!\n");
                    }
                    break;
                    
                case 0x13: // Ctrl+S (S键)
                    shell_display_control_char(key);
                    print_string(" - Save (not implemented)\n");
                    break;
                    
                case 0x14: // Ctrl+T (T键)
                    shell_display_control_char(key);
                    print_string(" - Transpose characters (not implemented)\n");
                    break;
                    
                case 0x18: // Ctrl+X (X键)
                    shell_display_control_char(key);
                    print_string(" - Cut (not implemented)\n");
                    break;
                    
                case 0x19: // Ctrl+Y (Y键)
                    shell_display_control_char(key);
                    print_string(" - Paste (not implemented)\n");
                    break;
                    
                case 0x1A: // Ctrl+Z (Z键)
                    shell_display_control_char(key);
                    print_string(" - Suspend (not implemented)\n");
                    break;
                    
                default:
                    // 显示未定义的控制字符
                    shell_display_control_char(key);
                    print_string(" - Unknown control sequence\n");
                    break;
            }
        } else {
            // 处理多键组合（最多10层）
            // 这里可以处理更复杂的组合键序列
            // 例如：Ctrl+Shift+A, Ctrl+Alt+X 等
        }
    }
    
    // 检查是否是Shift组合键
    if (modifiers & 0x01) { // Shift键被按下
        // 处理Shift组合键
    }
    
    // 检查是否是Alt组合键
    if (modifiers & 0x04) { // Alt键被按下
        // 处理Alt组合键
    }
    
    // 检查是否是复合修饰键（例如Ctrl+Shift, Alt+Ctrl等）
    if ((modifiers & 0x02) && (modifiers & 0x01)) { // Ctrl+Shift
        // 处理Ctrl+Shift组合键
    }
    
    if ((modifiers & 0x02) && (modifiers & 0x04)) { // Ctrl+Alt
        // 处理Ctrl+Alt组合键
    }
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
        keyboard_read_scancode();
        
        // 优先处理组合键事件（避免与普通字符冲突）
        int combo_processed = 0;
        if (keyboard_has_combo_event()) {
            combo_event_t event = keyboard_get_combo_event();
            
            if (event.type == COMBO_EVENT_KEY_DOWN) {
                // 检查是否是Ctrl组合键，但排除单独的修饰键
                uint8_t modifiers = keyboard_get_modifier_state();
                if ((modifiers & 0x02) && // Ctrl键被按下
                    event.scancode != KEY_CTRL && // 不是Ctrl键本身
                    event.scancode != KEY_SHIFT_LEFT && // 不是左Shift键
                    event.scancode != KEY_SHIFT_RIGHT && // 不是右Shift键
                    event.scancode != KEY_ALT) { // 不是Alt键
                    uint8_t sequence[1] = {event.scancode};
                    shell_handle_combo_sequence(sequence, 1, modifiers);
                    combo_processed = 1; // 标记组合键已处理
                    
                    // 清空键盘字符缓冲区，防止组合键对应的普通字符被重复处理
                    while (keyboard_has_char()) {
                        keyboard_get_char(); // 丢弃字符
                    }
                }
            }
        }
        
        // 处理普通字符（仅在组合键未处理时）
        if (!combo_processed && keyboard_has_char()) {
            char c = keyboard_get_char();
            
            if (c == '\n' || c == '\r') {
                // 回车键 - 处理命令
                print_char('\n');
                input_buffer[buffer_pos] = '\0';
                shell_process_command(input_buffer);
                buffer_pos = 0;
                cursor_pos = 0;
                keyboard_reset_combo_state();
                shell_print_prompt();
            } else if (c == '\b') {
                // 退格键
                if (cursor_pos > 0) {
                    cursor_pos--;
                    // 移动字符
                    for (int i = cursor_pos; i < buffer_pos; i++) {
                        input_buffer[i] = input_buffer[i + 1];
                    }
                    buffer_pos--;
                    // 重新显示剩余字符
                    print_char('\b');
                    for (int i = cursor_pos; i < buffer_pos; i++) {
                        print_char(input_buffer[i]);
                    }
                    print_char(' ');
                    for (int i = cursor_pos; i <= buffer_pos; i++) {
                        print_char('\b');
                    }
                }
                keyboard_reset_combo_state();
            } else if (c >= 32 && c <= 126) {
                // 可打印字符
                if (buffer_pos < SHELL_BUFFER_SIZE - 1) {
                    // 如果有字符在光标后面，需要移动它们
                    if (cursor_pos < buffer_pos) {
                        for (int i = buffer_pos; i > cursor_pos; i--) {
                            input_buffer[i] = input_buffer[i - 1];
                        }
                    }
                    input_buffer[cursor_pos] = c;
                    buffer_pos++;
                    cursor_pos++;
                    
                    // 显示字符和后面的字符
                    print_char(c);
                    for (int i = cursor_pos; i < buffer_pos; i++) {
                        print_char(input_buffer[i]);
                    }
                    // 移动光标回正确位置
                    for (int i = cursor_pos; i < buffer_pos; i++) {
                        print_char('\b');
                    }
                }
                keyboard_reset_combo_state();
            }
        }
        
        // 备用的简化组合键检测（保持兼容性）
        if (!combo_processed && keyboard_is_ctrl_pressed() && keyboard_has_char()) {
            char c = keyboard_get_char();
            
            // 检查是否是控制字符（ASCII 1-31）
            if (c >= 1 && c <= 31) {
                // 将控制字符转换为扫描码
                uint8_t scancode = c + 0x1E; // 简单的映射
                
                // 显示控制字符
                shell_display_control_char(scancode);
                
                // 处理组合键
                switch (c) {
                    case 3:  // Ctrl+C
                        print_char('\n');
                        buffer_pos = 0;
                        cursor_pos = 0;
                        shell_print_prompt();
                        break;
                    case 12: // Ctrl+L
                        print_char('\n');
                        clear_screen();
                        shell_print_prompt();
                        break;
                    case 21: // Ctrl+U
                        print_string(" - Delete to beginning of line\n");
                        if (cursor_pos > 0) {
                            for (int i = 0; i < cursor_pos; i++) {
                                print_char('\b');
                            }
                            int chars_to_delete = cursor_pos;
                            for (int i = 0; i < chars_to_delete; i++) {
                                print_char(' ');
                            }
                            for (int i = 0; i < chars_to_delete; i++) {
                                print_char('\b');
                            }
                            for (int i = cursor_pos; i < buffer_pos; i++) {
                                input_buffer[i - cursor_pos] = input_buffer[i];
                                print_char(input_buffer[i]);
                            }
                            for (int i = buffer_pos; i > buffer_pos - cursor_pos; i--) {
                                print_char(' ');
                            }
                            for (int i = buffer_pos; i > buffer_pos - cursor_pos; i--) {
                                print_char('\b');
                            }
                            buffer_pos -= cursor_pos;
                            cursor_pos = 0;
                        }
                        shell_print_prompt();
                        break;
                    default:
                        print_string(" - Unknown control sequence\n");
                        break;
                }
            }
        }
    }
}

// Shell初始化
void shell_init(void) {
    buffer_pos = 0;
    cursor_pos = 0;
    
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
