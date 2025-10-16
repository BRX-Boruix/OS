// Boruix OS 显示模块实现
// 处理VGA文本模式显示

#include "drivers/display.h"
#include "kernel/kernel.h"

// 当前光标位置
int cursor_x = 0;
int cursor_y = 0;

// 清屏函数
void clear_screen() {
    char* video_mem = (char*)VIDEO_MEMORY;
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT * 2; i += 2) {
        video_mem[i] = ' ';           // 字符
        video_mem[i + 1] = WHITE;     // 颜色
    }
    cursor_x = 0;
    cursor_y = 0;
}

// 设置光标位置
void set_cursor(int x, int y) {
    cursor_x = x;
    cursor_y = y;
}

// 打印字符
void print_char(char c) {
    char* video_mem = (char*)VIDEO_MEMORY;
    
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else {
        int pos = (cursor_y * SCREEN_WIDTH + cursor_x) * 2;
        video_mem[pos] = c;
        video_mem[pos + 1] = WHITE;
        cursor_x++;
    }
    
    // 换行处理
    if (cursor_x >= SCREEN_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    // 滚动屏幕
    if (cursor_y >= SCREEN_HEIGHT) {
        // 简单的滚动：将所有行向上移动一行
        for (int y = 0; y < SCREEN_HEIGHT - 1; y++) {
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                int src_pos = ((y + 1) * SCREEN_WIDTH + x) * 2;
                int dst_pos = (y * SCREEN_WIDTH + x) * 2;
                video_mem[dst_pos] = video_mem[src_pos];
                video_mem[dst_pos + 1] = video_mem[src_pos + 1];
            }
        }
        
        // 清空最后一行
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int pos = ((SCREEN_HEIGHT - 1) * SCREEN_WIDTH + x) * 2;
            video_mem[pos] = ' ';
            video_mem[pos + 1] = WHITE;
        }
        
        cursor_y = SCREEN_HEIGHT - 1;
    }
}

// 打印字符串
void print_string(const char* str) {
    while (*str) {
        print_char(*str);
        str++;
    }
}

// 简单的延迟函数
void delay(int count) {
    for (volatile int i = 0; i < count; i++) {
        // 空循环
    }
}
