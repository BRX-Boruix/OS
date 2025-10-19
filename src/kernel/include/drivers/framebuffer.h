// Framebuffer驱动头文件

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "kernel/types.h"
#include "kernel/limine.h"

// 初始化framebuffer
void fb_init(struct limine_framebuffer *framebuffer);

// 基本输出函数
void fb_clear_screen(void);
void fb_putchar(char c);
void fb_print_string(const char *str);
void fb_print_hex(uint64_t value);
void fb_print_dec(uint32_t value);

// 颜色控制
void fb_set_color(uint32_t foreground, uint32_t background);

// 获取信息
void fb_get_resolution(uint32_t *width, uint32_t *height);

#endif

