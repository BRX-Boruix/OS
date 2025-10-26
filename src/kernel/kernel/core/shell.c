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
        // 检查是否有输入事件需要处理
        if (!keyboard_has_char() && !keyboard_has_combo_event()) {
            // 没有输入事件，使用hlt等待中断（节省CPU）
            __asm__ volatile("hlt");
            continue;  // 被唤醒后重新检查
        }
        
        // 确保中断处理完成后继续执行
        __asm__ volatile("" ::: "memory");
        
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
            
            // 处理特殊键（翻页键等）
            if (c == 0x01) {  // Ctrl+A (Page Down)
                terminal_history_page_down();
                keyboard_reset_combo_state();
            } else if (c == 0x02) {  // Ctrl+B (Page Up)
                terminal_history_page_up();
                keyboard_reset_combo_state();
            } else if (c == 0x05) {  // Ctrl+E (Up Arrow)
                terminal_history_scroll_up();
                keyboard_reset_combo_state();
            } else if (c == 0x06) {  // Ctrl+F (Down Arrow)
                terminal_history_scroll_down();
                keyboard_reset_combo_state();
            } else if (c == '\n' || c == '\r') {
                // 回车键 - 处理命令
                print_char('\n');
                input_buffer[buffer_pos] = '\0';
                
                // 将输入行添加到历史缓冲区
                terminal_history_add_line(input_buffer);
                
                // 启用输出捕获
                terminal_enable_output_capture();
                
                shell_process_command(input_buffer);
                
                // 完成输出捕获
                terminal_finish_output_capture();
                
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
        
        // 定期刷新显示
        display_flush();
    }
}

// Shell初始化
// Shell初始化函数
void shell_init(void) {
    buffer_pos = 0;
    cursor_pos = 0;
    
    // 初始化键盘驱动
    keyboard_init();
    
    // 初始化终端历史缓冲区
    terminal_history_init();
    
    // 添加大量测试数据到历史缓冲区（超过屏幕高度）
    for (int i = 1; i <= 60; i++) {
        char test_line[128];
        // 创建测试行
        int j = 0;
        test_line[j++] = 'T';
        test_line[j++] = 'e';
        test_line[j++] = 's';
        test_line[j++] = 't';
        test_line[j++] = ' ';
        test_line[j++] = 'l';
        test_line[j++] = 'i';
        test_line[j++] = 'n';
        test_line[j++] = 'e';
        test_line[j++] = ' ';
        
        // 添加行号
        if (i >= 100) {
            test_line[j++] = '0' + (i / 100);
            test_line[j++] = '0' + ((i / 10) % 10);
            test_line[j++] = '0' + (i % 10);
        } else if (i >= 10) {
            test_line[j++] = '0' + (i / 10);
            test_line[j++] = '0' + (i % 10);
        } else {
            test_line[j++] = '0' + i;
        }
        
        test_line[j++] = ':';
        test_line[j++] = ' ';
        test_line[j++] = 'T';
        test_line[j++] = 'h';
        test_line[j++] = 'i';
        test_line[j++] = 's';
        test_line[j++] = ' ';
        test_line[j++] = 'i';
        test_line[j++] = 's';
        test_line[j++] = ' ';
        test_line[j++] = 't';
        test_line[j++] = 'e';
        test_line[j++] = 's';
        test_line[j++] = 't';
        test_line[j++] = ' ';
        test_line[j++] = 'd';
        test_line[j++] = 'a';
        test_line[j++] = 't';
        test_line[j++] = 'a';
        test_line[j++] = ' ';
        test_line[j++] = 'f';
        test_line[j++] = 'o';
        test_line[j++] = 'r';
        test_line[j++] = ' ';
        test_line[j++] = 's';
        test_line[j++] = 'c';
        test_line[j++] = 'r';
        test_line[j++] = 'o';
        test_line[j++] = 'l';
        test_line[j++] = 'l';
        test_line[j++] = 'i';
        test_line[j++] = 'n';
        test_line[j++] = 'g';
        test_line[j++] = ' ';
        test_line[j++] = 'f';
        test_line[j++] = 'u';
        test_line[j++] = 'n';
        test_line[j++] = 'c';
        test_line[j++] = 't';
        test_line[j++] = 'i';
        test_line[j++] = 'o';
        test_line[j++] = 'n';
        test_line[j++] = 'a';
        test_line[j++] = 'l';
        test_line[j++] = 'i';
        test_line[j++] = 't';
        test_line[j++] = 'y';
        test_line[j] = '\0';
        
        terminal_history_add_line(test_line);
    }
    
    // Shell初始化完成
    print_string("[SHELL] Shell initialized\n");
    
    // 启用中断
    print_string("[SHELL] Enabling interrupts...\n");
    interrupts_enable();
    print_string("[SHELL] Interrupts enabled\n");
    print_string("\n");
    
    print_string("========================================\n");
    print_string("Boruix Shell\n");
    print_string("Type 'help' for available commands.\n");
    print_string("\n");
    print_string("========================================\n");
    print_string("https://github.com/BRX-Boruix/OS\n");
    print_string("https://os.boruix.thelang.cn\n");
    print_string("========================================\n");
    print_string("\n");
}
