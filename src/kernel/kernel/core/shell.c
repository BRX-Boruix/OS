// Boruix OS Shell - 重构后的主shell文件
// 使用模块化的设计

#include "kernel/shell.h"
#include "kernel/interrupt.h"
#include "drivers/display.h"
#include "drivers/keyboard.h"
#include "../shell/utils/string.h"
#include "../shell/utils/combo.h"
#include "../shell/commands/command.h"
#include "../shell/builtin/builtin.h"
#include "../shell/utils/system.h"

// 全局变量
static char input_buffer[SHELL_BUFFER_SIZE];
static int buffer_pos = 0;
static int cursor_pos = 0;  // 光标在输入缓冲区中的位置

// 显示提示符
void shell_print_prompt(void) {
    print_string(SHELL_PROMPT);
}

// 处理组合键的具体实现（需要访问shell的全局变量）
void shell_handle_combo_actions(uint8_t key, uint8_t modifiers) {
    if (modifiers & 0x02) { // Ctrl键被按下
        switch (key) {
            case 0x2E: // Ctrl+C (C键)
                shell_display_control_char(key);
                print_char('\n');
                buffer_pos = 0;
                cursor_pos = 0;
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
                
            default:
                // 使用通用的组合键处理
                uint8_t sequence[1] = {key};
                shell_handle_combo_sequence(sequence, 1, modifiers);
                break;
        }
    }
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
                    shell_handle_combo_actions(event.scancode, modifiers);
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
// Shell初始化函数
void shell_init(void) {
    buffer_pos = 0;
    cursor_pos = 0;
    
    // 初始化键盘驱动
    keyboard_init();
    
    // Shell初始化完成，现在可以安全地启用中断
    print_string("[SHELL] Shell initialized, enabling interrupts...\n");
    interrupts_enable();
    print_string("[SHELL] Interrupts enabled\n\n");
    
    print_string("========================================\n");
    print_string("Boruix Shell\n");
    print_string("Type 'help' for available commands.\n");
    print_string("\n");
    print_string("========================================\n");
    print_string("https://github.com/borui-x/os\n");
    print_string("https://boruix.thelang.cn\n");
    print_string("========================================\n");
    print_string("\n");
}
