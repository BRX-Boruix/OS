// Boruix OS 显示模块头文件
// 处理VGA文本模式显示

#ifndef BORUIX_DISPLAY_H
#define BORUIX_DISPLAY_H

#include "kernel/types.h"

// 显示函数声明
void clear_screen(void);
void set_cursor(int x, int y);
void print_char(char c);
void print_string(const char* str);
void delay(int count);

// 辅助打印函数
void print_hex(uint32_t value);
void print_dec(uint32_t value);

#endif // BORUIX_DISPLAY_H
