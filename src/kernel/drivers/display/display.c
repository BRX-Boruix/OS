// Boruix OS 显示驱动 - Framebuffer适配层
// 兼容原VGA接口，底层使用framebuffer实现

#include "drivers/display.h"
#include "kernel/limine.h"
#include "font.h"

static struct limine_framebuffer *fb = NULL;
static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;
static uint32_t fg_color = 0xFFFFFF;
static uint32_t bg_color = 0x000000;

// 屏幕滚动相关变量
static uint32_t screen_width_chars = 0;  // 屏幕宽度（字符数）
static uint32_t screen_height_chars = 0; // 屏幕高度（字符数）
static uint32_t scroll_offset = 0;       // 当前滚动偏移量

// 初始化显示系统（由内核调用）
void display_init(struct limine_framebuffer *framebuffer) {
    fb = framebuffer;
    cursor_x = 0;
    cursor_y = 0;
    scroll_offset = 0;
    
    // 计算屏幕尺寸（字符数）
    screen_width_chars = fb->width / font_get_width();
    screen_height_chars = fb->height / font_get_height();
}

static void putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb || x >= fb->width || y >= fb->height) return;
    uint32_t *pixel = (uint32_t *)((uint8_t *)fb->address + y * fb->pitch + x * 4);
    *pixel = color;
}

void clear_screen(void) {
    if (!fb) return;
    for (uint32_t i = 0; i < fb->height * fb->width; i++) {
        ((uint32_t *)fb->address)[i] = bg_color;
    }
    cursor_x = 0;
    cursor_y = 0;
    scroll_offset = 0;
}


void print_char(char c) {
    if (!fb) return;
    
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += font_get_height();
        if (cursor_y + font_get_height() > fb->height) {
            // 当光标超出屏幕时，向上滚动而不是重置
            scroll_screen_up();
            cursor_y = fb->height - font_get_height(); // 将光标放在最后一行
        }
        return;
    }
    
    if (c == '\r') {
        cursor_x = 0;
        return;
    }
    
    if (c == '\b') {
        // 退格键：向左移动光标并清除字符
        if (cursor_x >= font_get_width()) {
            cursor_x -= font_get_width();
            // 清除当前位置的字符（用背景色填充）
            for (int row = 0; row < font_get_height(); row++) {
                for (int col = 0; col < font_get_width(); col++) {
                    putpixel(cursor_x + col, cursor_y + row, bg_color);
                }
            }
        }
        return;
    }
    
    // 绘制字符
    const uint8_t *glyph = font_get_glyph((uint8_t)c);
    
    for (int row = 0; row < font_get_height(); row++) {
        for (int col = 0; col < font_get_width(); col++) {
            // 反转位顺序：从右到左读取（0表示最左边像素）
            uint32_t color = (glyph[row] & (1 << col)) ? fg_color : bg_color;
            putpixel(cursor_x + col, cursor_y + row, color);
        }
    }
    
    cursor_x += font_get_width();
    if (cursor_x + font_get_width() > fb->width) {
        cursor_x = 0;
        cursor_y += font_get_height();
        if (cursor_y + font_get_height() > fb->height) {
            // 当光标超出屏幕时，向上滚动而不是重置
            scroll_screen_up();
            cursor_y = fb->height - font_get_height(); // 将光标放在最后一行
        }
    }
}

void print_string(const char *str) {
    while (*str) {
        print_char(*str++);
    }
}

void print_hex(uint64_t value) {
    char hex_chars[] = "0123456789ABCDEF";
    print_string("0x");
    
    int started = 0;
    for (int i = 60; i >= 0; i -= 4) {
        uint8_t digit = (value >> i) & 0xF;
        if (digit != 0 || started || i == 0) {
            print_char(hex_chars[digit]);
            started = 1;
        }
    }
}

void print_dec(uint32_t value) {
    if (value == 0) {
        print_char('0');
        return;
    }
    
    char buffer[12];
    int i = 0;
    
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    while (--i >= 0) {
        print_char(buffer[i]);
    }
}

void set_color(uint8_t fg, uint8_t bg) {
    // VGA颜色到RGB转换（简化版）
    uint32_t colors[16] = {
        0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
        0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
        0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
        0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
    };
    
    if (fg < 16) fg_color = colors[fg];
    if (bg < 16) bg_color = colors[bg];
}

// 屏幕滚动相关函数实现
void scroll_screen_up(void) {
    if (!fb) return;
    
    // 将整个屏幕内容向上移动一行（字体高度像素）
    uint32_t line_size = fb->pitch * font_get_height(); // 一行的大小（字节）
    
    // 从第二行开始，将内容向上复制
    uint8_t *src = (uint8_t *)fb->address + line_size;
    uint8_t *dst = (uint8_t *)fb->address;
    
    // 使用更安全的内存移动方式
    // 逐行复制，避免内存重叠问题
    for (uint32_t line = 0; line < (fb->height - font_get_height()); line += font_get_height()) {
        uint8_t *src_line = src + line * fb->pitch;
        uint8_t *dst_line = dst + line * fb->pitch;
        for (uint32_t i = 0; i < line_size; i++) {
            dst_line[i] = src_line[i];
        }
    }
    
    // 清除最后一行
    uint8_t *last_line = (uint8_t *)fb->address + (fb->height - font_get_height()) * fb->pitch;
    for (uint32_t y = 0; y < font_get_height(); y++) {
        for (uint32_t x = 0; x < fb->width; x++) {
            uint32_t *pixel = (uint32_t *)(last_line + y * fb->pitch + x * 4);
            *pixel = bg_color;
        }
    }
    
    // 更新滚动偏移量
    scroll_offset++;
}

// 获取屏幕尺寸信息
uint32_t get_screen_width_chars(void) {
    return screen_width_chars;
}

uint32_t get_screen_height_chars(void) {
    return screen_height_chars;
}

uint32_t get_scroll_offset(void) {
    return scroll_offset;
}

// 设置光标位置
void set_cursor(int x, int y) {
    if (!fb) return;
    
    // 将字符坐标转换为像素坐标
    cursor_x = x * font_get_width();
    cursor_y = y * font_get_height();
    
    // 确保光标位置在有效范围内
    if (cursor_x >= fb->width) cursor_x = fb->width - font_get_width();
    if (cursor_y >= fb->height) cursor_y = fb->height - font_get_height();
}
