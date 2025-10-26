// Boruix OS 显示驱动 - 使用Flanterm库
// 基于flanterm的现代终端实现

#include "drivers/display.h"
#include "kernel/limine.h"
#include "../flanterm/flanterm.h"
#include "../flanterm/flanterm_backends/fb.h"
#include "../../kernel/shell/utils/string.h"

static struct flanterm_context *ft_ctx = NULL;

// 显示锁函数声明
extern void display_acquire_lock(void);
extern void display_release_lock(void);

// 终端历史缓冲区管理
#define TERMINAL_HISTORY_SIZE 1000  // 历史行数
#define TERMINAL_LINE_SIZE 256      // 每行最大字符数

// 历史缓冲区结构
typedef struct {
    char lines[TERMINAL_HISTORY_SIZE][TERMINAL_LINE_SIZE];
    int line_count;
    int current_line;
    int scroll_offset;  // 当前滚动偏移
    int max_scroll_offset;  // 最大滚动偏移
} terminal_history_t;

static terminal_history_t terminal_history = {0};

// 输出捕获系统
static int output_capture_enabled = 0;
static char captured_output[1024];
static int captured_output_pos = 0;

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
    
    display_acquire_lock();
    // 使用ANSI转义序列清屏
    flanterm_write(ft_ctx, "\033[2J", 4);
    flanterm_write(ft_ctx, "\033[H", 3);  // 移动光标到左上角
    flanterm_flush(ft_ctx);
    display_release_lock();
}

void set_cursor(int x, int y) {
    if (!ft_ctx) return;
    
    display_acquire_lock();
    
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
    
    display_release_lock();
}

void print_char(char c) {
    if (!ft_ctx) return;
    
    display_acquire_lock();
    
    // 如果启用了输出捕获，将字符添加到历史缓冲区
    if (output_capture_enabled) {
        char str[2] = {c, '\0'};
        terminal_capture_output(str);
    }
    
    flanterm_write(ft_ctx, &c, 1);
    // 不立即flush，让flanterm内部处理
    
    display_release_lock();
}

void print_string(const char* str) {
    if (!ft_ctx || !str) return;
    
    display_acquire_lock();
    
    // 如果启用了输出捕获，将输出添加到历史缓冲区
    if (output_capture_enabled) {
        terminal_capture_output(str);
    }
    
    flanterm_write(ft_ctx, str, shell_strlen(str));
    // 不立即flush，让flanterm内部处理
    
    display_release_lock();
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
    
    // print_string已经有锁了，不需要再加锁
    print_string(buffer);
}

// 颜色设置（flanterm支持ANSI颜色）
void set_color(uint8_t fg, uint8_t bg) {
    if (!ft_ctx) return;
    
    display_acquire_lock();
    
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
    
    display_release_lock();
}

// 手动刷新显示
void display_flush(void) {
    if (!ft_ctx) return;
    
    display_acquire_lock();
    flanterm_flush(ft_ctx);
    display_release_lock();
}

// 获取flanterm上下文（用于高级功能）
struct flanterm_context* get_flanterm_context(void) {
    return ft_ctx;
}

// === 终端历史缓冲区管理函数 ===

// 初始化历史缓冲区
void terminal_history_init(void) {
    terminal_history.line_count = 0;
    terminal_history.current_line = 0;
    terminal_history.scroll_offset = 0;
    terminal_history.max_scroll_offset = 0;
    
    // 清空所有行
    for (int i = 0; i < TERMINAL_HISTORY_SIZE; i++) {
        terminal_history.lines[i][0] = '\0';
    }
}

// 添加一行到历史缓冲区
void terminal_history_add_line(const char* line) {
    if (!line || terminal_history.line_count >= TERMINAL_HISTORY_SIZE) {
        return;
    }
    
    // 复制行内容
    int i = 0;
    while (line[i] != '\0' && i < TERMINAL_LINE_SIZE - 1) {
        terminal_history.lines[terminal_history.line_count][i] = line[i];
        i++;
    }
    terminal_history.lines[terminal_history.line_count][i] = '\0';
    
    terminal_history.line_count++;
    terminal_history.current_line = terminal_history.line_count;
    
    // 更新最大滚动偏移
    uint32_t screen_height = get_screen_height_chars();
    if (terminal_history.line_count > screen_height) {
        terminal_history.max_scroll_offset = terminal_history.line_count - screen_height;
    } else {
        terminal_history.max_scroll_offset = 0;
    }
}

// 向上翻页
void terminal_history_page_up(void) {
    if (terminal_history.scroll_offset < terminal_history.max_scroll_offset) {
        uint32_t screen_height = get_screen_height_chars();
        terminal_history.scroll_offset += screen_height;
        
        // 确保不超过最大偏移
        if (terminal_history.scroll_offset > terminal_history.max_scroll_offset) {
            terminal_history.scroll_offset = terminal_history.max_scroll_offset;
        }
        
        // 重新绘制屏幕
        terminal_history_redraw();
    }
}

// 向下翻页
void terminal_history_page_down(void) {
    if (terminal_history.scroll_offset > 0) {
        uint32_t screen_height = get_screen_height_chars();
        terminal_history.scroll_offset -= screen_height;
        
        // 确保不小于0
        if (terminal_history.scroll_offset < 0) {
            terminal_history.scroll_offset = 0;
        }
        
        // 重新绘制屏幕
        terminal_history_redraw();
    }
}

// 向上滚动一行
void terminal_history_scroll_up(void) {
    if (terminal_history.scroll_offset < terminal_history.max_scroll_offset) {
        terminal_history.scroll_offset++;
        terminal_history_redraw();
    }
}

// 向下滚动一行
void terminal_history_scroll_down(void) {
    if (terminal_history.scroll_offset > 0) {
        terminal_history.scroll_offset--;
        terminal_history_redraw();
    }
}

// 重新绘制历史内容
void terminal_history_redraw(void) {
    if (!ft_ctx) return;
    
    // 清屏
    clear_screen();
    
    // 计算要显示的行范围
    uint32_t screen_height = get_screen_height_chars();
    int start_line = terminal_history.scroll_offset;
    int end_line = start_line + screen_height;
    
    // 确保不超出历史范围
    if (end_line > terminal_history.line_count) {
        end_line = terminal_history.line_count;
    }
    
    // 显示历史行
    for (int i = start_line; i < end_line; i++) {
        print_string(terminal_history.lines[i]);
        print_char('\n');
    }
    
    // 显示提示符（如果不在历史模式）
    if (terminal_history.scroll_offset == 0) {
        // 这里可以显示当前提示符
    }
}

// 获取当前滚动偏移
int terminal_history_get_scroll_offset(void) {
    return terminal_history.scroll_offset;
}

// 获取最大滚动偏移
int terminal_history_get_max_scroll_offset(void) {
    return terminal_history.max_scroll_offset;
}

// 检查是否在历史模式
int terminal_history_is_in_history(void) {
    return terminal_history.scroll_offset > 0;
}

// 启用输出捕获
void terminal_enable_output_capture(void) {
    output_capture_enabled = 1;
    captured_output_pos = 0;
    captured_output[0] = '\0';
}

// 禁用输出捕获
void terminal_disable_output_capture(void) {
    output_capture_enabled = 0;
}

// 捕获输出到历史缓冲区
void terminal_capture_output(const char* str) {
    if (!output_capture_enabled || !str) return;
    
    // 将输出添加到捕获缓冲区
    int i = 0;
    while (str[i] != '\0' && captured_output_pos < 1023) {
        if (str[i] == '\n') {
            // 遇到换行符，将当前行添加到历史缓冲区
            captured_output[captured_output_pos] = '\0';
            if (captured_output_pos > 0) {
                terminal_history_add_line(captured_output);
            }
            captured_output_pos = 0;
            captured_output[0] = '\0';
        } else {
            captured_output[captured_output_pos] = str[i];
            captured_output_pos++;
        }
        i++;
    }
}

// 完成输出捕获
void terminal_finish_output_capture(void) {
    if (output_capture_enabled && captured_output_pos > 0) {
        captured_output[captured_output_pos] = '\0';
        terminal_history_add_line(captured_output);
        captured_output_pos = 0;
    }
    terminal_disable_output_capture();
}