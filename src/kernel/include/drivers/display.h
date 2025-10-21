// Boruix OS 显示模块头文件
// Framebuffer适配层（兼容原VGA接口）

#ifndef BORUIX_DISPLAY_H
#define BORUIX_DISPLAY_H

#include "kernel/types.h"
#include "kernel/limine.h"

// 初始化函数（内核启动时调用）
void display_init(struct limine_framebuffer *framebuffer);

// 显示函数声明（兼容VGA接口）
void clear_screen(void);
void set_cursor(int x, int y);
void print_char(char c);
void print_string(const char* str);
void delay(int count);

// 屏幕滚动相关函数
void scroll_screen_up(void);
uint32_t get_screen_width_chars(void);
uint32_t get_screen_height_chars(void);
uint32_t get_scroll_offset(void);

// 辅助打印函数
void print_hex(uint64_t value);
void print_dec(uint32_t value);

// 颜色设置（VGA兼容）
void set_color(uint8_t fg, uint8_t bg);

// 手动刷新显示
void display_flush(void);

// 终端历史缓冲区管理函数
void terminal_history_init(void);
void terminal_history_add_line(const char* line);
void terminal_history_page_up(void);
void terminal_history_page_down(void);
void terminal_history_scroll_up(void);
void terminal_history_scroll_down(void);
void terminal_history_redraw(void);
int terminal_history_get_scroll_offset(void);
int terminal_history_get_max_scroll_offset(void);
int terminal_history_is_in_history(void);

// 输出捕获系统
void terminal_enable_output_capture(void);
void terminal_disable_output_capture(void);
void terminal_capture_output(const char* str);
void terminal_finish_output_capture(void);

#endif // BORUIX_DISPLAY_H
