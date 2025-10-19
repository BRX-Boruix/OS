// Framebuffer驱动 - 替代VGA文本模式

#include "kernel/types.h"
#include "kernel/limine.h"

// 完整的8x8字体
#include "fb_font_8x8.h"

static struct limine_framebuffer *fb = NULL;
static uint32_t cursor_x = 0, cursor_y = 0;
static uint32_t fg_color = 0xFFFFFF;
static uint32_t bg_color = 0x000000;

void fb_init(struct limine_framebuffer *framebuffer) {
    fb = framebuffer;
    cursor_x = 0;
    cursor_y = 0;
}

static void putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb || x >= fb->width || y >= fb->height) return;
    uint32_t *pixel = (uint32_t *)((uint8_t *)fb->address + y * fb->pitch + x * 4);
    *pixel = color;
}

void fb_clear_screen(void) {
    if (!fb) return;
    for (uint32_t i = 0; i < fb->height * fb->width; i++) {
        ((uint32_t *)fb->address)[i] = bg_color;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void fb_putchar(char c) {
    if (!fb) return;
    
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += 8;
        if (cursor_y + 8 > fb->height) cursor_y = 0;
        return;
    }
    
    if (c == '\r') {
        cursor_x = 0;
        return;
    }
    
    // 绘制字符
    uint8_t idx = (uint8_t)c;
    if (idx >= 128) idx = '?';
    const uint8_t *glyph = font_8x8_basic[idx];
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            uint32_t color = (glyph[row] & (0x80 >> col)) ? fg_color : bg_color;
            putpixel(cursor_x + col, cursor_y + row, color);
        }
    }
    
    cursor_x += 8;
    if (cursor_x + 8 > fb->width) {
        cursor_x = 0;
        cursor_y += 8;
        if (cursor_y + 8 > fb->height) cursor_y = 0;
    }
}

void fb_print_string(const char *str) {
    while (*str) {
        fb_putchar(*str++);
    }
}

void fb_print_hex(uint64_t value) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[17];
    int i = 15;
    
    buffer[16] = '\0';
    do {
        buffer[i--] = hex_chars[value & 0xF];
        value >>= 4;
    } while (value && i >= 0);
    
    fb_print_string("0x");
    fb_print_string(&buffer[i + 1]);
}

void fb_print_dec(uint32_t value) {
    if (value == 0) {
        fb_putchar('0');
        return;
    }
    
    char buffer[12];
    int i = 0;
    
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    while (--i >= 0) {
        fb_putchar(buffer[i]);
    }
}

void fb_set_color(uint32_t foreground, uint32_t background) {
    fg_color = foreground;
    bg_color = background;
}

void fb_get_resolution(uint32_t *width, uint32_t *height) {
    if (fb && width && height) {
        *width = fb->width;
        *height = fb->height;
    }
}

