// Boruix OS 显示驱动 - 使用Flanterm库
// 基于flanterm的现代终端实现

#include "drivers/display.h"
#include "kernel/limine.h"
#include "../flanterm/flanterm.h"
#include "../flanterm/flanterm_backends/fb.h"
#include "../../kernel/shell/utils/string.h"

static struct flanterm_context *ft_ctx = NULL;

// 初始化显示系统（由内核调用）
void display_init(struct limine_framebuffer *framebuffer) {
    if (!framebuffer) return;
    
    // 初始化flanterm framebuffer后端
    ft_ctx = flanterm_fb_init(
        NULL,  // malloc函数（使用默认）
        NULL,  // free函数（使用默认）
        (uint32_t*)framebuffer->address,
        framebuffer->width,
        framebuffer->height,
        framebuffer->pitch,
        framebuffer->red_mask_size,
        framebuffer->red_mask_shift,
        framebuffer->green_mask_size,
        framebuffer->green_mask_shift,
        framebuffer->blue_mask_size,
        framebuffer->blue_mask_shift,
        NULL,  // canvas
        NULL,  // ansi_colours
        NULL,  // ansi_bright_colours
        NULL,  // default_bg
        NULL,  // default_fg
        NULL,  // default_bg_bright
        NULL,  // default_fg_bright
        NULL,  // font
        0,     // font_width
        0,     // font_height
        1,     // font_spacing
        0,     // font_scale_x (自动)
        0,     // font_scale_y (自动)
        0      // margin
    );
}

void clear_screen(void) {
    if (!ft_ctx) return;
    // 使用ANSI转义序列清屏
    flanterm_write(ft_ctx, "\033[2J", 4);
    flanterm_write(ft_ctx, "\033[H", 3);  // 移动光标到左上角
    flanterm_flush(ft_ctx);
}

void set_cursor(int x, int y) {
    if (!ft_ctx) return;
    // 使用ANSI转义序列设置光标位置
    char buffer[32];
    int i = 0;
    
    buffer[i++] = '\033';
    buffer[i++] = '[';
    
    // 转换y坐标
    if (y + 1 >= 10) {
        buffer[i++] = '0' + ((y + 1) / 10);
        buffer[i++] = '0' + ((y + 1) % 10);
    } else {
        buffer[i++] = '0' + (y + 1);
    }
    
    buffer[i++] = ';';
    
    // 转换x坐标
    if (x + 1 >= 10) {
        buffer[i++] = '0' + ((x + 1) / 10);
        buffer[i++] = '0' + ((x + 1) % 10);
    } else {
        buffer[i++] = '0' + (x + 1);
    }
    
    buffer[i++] = 'H';
    
    flanterm_write(ft_ctx, buffer, i);
    flanterm_flush(ft_ctx);
}

void print_char(char c) {
    if (!ft_ctx) return;
    flanterm_write(ft_ctx, &c, 1);
    // 不立即flush，让flanterm内部处理
}

void print_string(const char* str) {
    if (!ft_ctx || !str) return;
    flanterm_write(ft_ctx, str, shell_strlen(str));
    // 不立即flush，让flanterm内部处理
}

void delay(int count) {
    // 简单的延迟实现
    for (volatile int i = 0; i < count * 1000; i++);
}

// 屏幕滚动相关函数（flanterm自动处理）
void scroll_screen_up(void) {
    // flanterm自动处理滚动，无需手动实现
}

uint32_t get_screen_width_chars(void) {
    if (!ft_ctx) return 0;
    size_t cols, rows;
    flanterm_get_dimensions(ft_ctx, &cols, &rows);
    return (uint32_t)cols;
}

uint32_t get_screen_height_chars(void) {
    if (!ft_ctx) return 0;
    size_t cols, rows;
    flanterm_get_dimensions(ft_ctx, &cols, &rows);
    return (uint32_t)rows;
}

uint32_t get_scroll_offset(void) {
    // flanterm内部管理滚动，返回0表示当前实现
    return 0;
}

// 辅助打印函数
void print_hex(uint64_t value) {
    char buffer[32];
    char hex_chars[] = "0123456789ABCDEF";
    int i = 0;
    
    buffer[i++] = '0';
    buffer[i++] = 'x';
    
    if (value == 0) {
        buffer[i++] = '0';
    } else {
        // 从高位开始转换
        int started = 0;
        for (int shift = 60; shift >= 0; shift -= 4) {
            uint8_t digit = (value >> shift) & 0xF;
            if (digit != 0 || started || shift == 0) {
                buffer[i++] = hex_chars[digit];
                started = 1;
            }
        }
    }
    
    buffer[i] = '\0';
    print_string(buffer);
}

void print_dec(uint32_t value) {
    char buffer[32];
    int i = 0;
    
    if (value == 0) {
        buffer[i++] = '0';
    } else {
        // 从低位开始转换
        char temp[32];
        int j = 0;
        while (value > 0) {
            temp[j++] = '0' + (value % 10);
            value /= 10;
        }
        // 反转
        while (j > 0) {
            buffer[i++] = temp[--j];
        }
    }
    
    buffer[i] = '\0';
    print_string(buffer);
}

// 颜色设置（flanterm支持ANSI颜色）
void set_color(uint8_t fg, uint8_t bg) {
    if (!ft_ctx) return;
    
    // 使用ANSI转义序列设置颜色
    char buffer[32];
    int i = 0;
    
    buffer[i++] = '\033';
    buffer[i++] = '[';
    
    // 设置前景色
    int fg_val = 30 + fg;
    if (fg_val >= 10) {
        buffer[i++] = '0' + (fg_val / 10);
        buffer[i++] = '0' + (fg_val % 10);
    } else {
        buffer[i++] = '0' + fg_val;
    }
    
    buffer[i++] = ';';
    
    // 设置背景色
    int bg_val = 40 + bg;
    if (bg_val >= 10) {
        buffer[i++] = '0' + (bg_val / 10);
        buffer[i++] = '0' + (bg_val % 10);
    } else {
        buffer[i++] = '0' + bg_val;
    }
    
    buffer[i++] = 'm';
    
    flanterm_write(ft_ctx, buffer, i);
    flanterm_flush(ft_ctx);
}

// 手动刷新显示
void display_flush(void) {
    if (!ft_ctx) return;
    flanterm_flush(ft_ctx);
}

// 获取flanterm上下文（用于高级功能）
struct flanterm_context* get_flanterm_context(void) {
    return ft_ctx;
}