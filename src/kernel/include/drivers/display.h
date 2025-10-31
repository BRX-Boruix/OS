// Boruix OS 显示模块头文件
// 统一显示驱动接口 - 基于Flanterm库的现代终端仿真

#ifndef BORUIX_DISPLAY_H
#define BORUIX_DISPLAY_H

#include "kernel/types.h"
#include "kernel/limine.h"

// Flanterm前向声明
struct flanterm_context;

// === 基本初始化函数 ===

// 初始化显示系统（由内核启动时调用）
void display_init(struct limine_framebuffer *framebuffer);

// === VGA兼容接口函数 ===

// 屏幕清理
void clear_screen(void);

// 光标控制
void set_cursor(int x, int y);

// 字符输出
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

// 颜色设置（ANSI颜色支持）
void set_color(uint8_t fg, uint8_t bg);

// 手动刷新显示
void display_flush(void);

// === 终端历史缓冲区管理 ===

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

// === 输出捕获系统 ===

void terminal_enable_output_capture(void);
void terminal_disable_output_capture(void);
void terminal_capture_output(const char* str);
void terminal_finish_output_capture(void);

// === 高级接口 - 获取底层Flanterm上下文 ===

// 获取Flanterm上下文用于高级功能
struct flanterm_context* get_flanterm_context(void);

#endif // BORUIX_DISPLAY_H
