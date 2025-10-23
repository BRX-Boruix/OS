// Boruix OS TTY内核日志系统实现
// 提供内核输出和日志功能

#include "kernel/tty.h"
#include "kernel/memory.h"
#include "../../kernel/shell/utils/string.h"

// 内核日志缓冲区
static char klog_buffer[KMSG_BUFFER_SIZE];
static size_t klog_head = 0;
static size_t klog_tail = 0;
static bool klog_initialized = false;

// 日志级别颜色映射
static const char* log_level_colors[] = {
    "\033[36m",  // DEBUG - 青色
    "\033[32m",  // INFO - 绿色
    "\033[33m",  // WARN - 黄色
    "\033[31m",  // ERROR - 红色
    "\033[35m"   // FATAL - 品红
};

static const char* log_level_names[] = {
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR",
    "FATAL"
};

// 初始化内核日志系统
void klog_init(void) {
    if (klog_initialized) return;
    
    klog_head = 0;
    klog_tail = 0;
    klog_initialized = true;
}

// 检查缓冲区是否为空
static bool klog_is_empty(void) {
    return klog_head == klog_tail;
}

// 检查缓冲区是否已满
static bool klog_is_full(void) {
    return (klog_head + 1) % KMSG_BUFFER_SIZE == klog_tail;
}

// 向日志缓冲区写入字符
void klog_putc(char c) {
    if (!klog_initialized) klog_init();
    
    klog_buffer[klog_head] = c;
    klog_head = (klog_head + 1) % KMSG_BUFFER_SIZE;
    
    // 如果缓冲区满了，移动尾指针
    if (klog_is_full()) {
        klog_tail = (klog_tail + 1) % KMSG_BUFFER_SIZE;
    }
}

// 向日志缓冲区写入字符串
void klog_write(const char *str) {
    if (!str) return;
    
    while (*str) {
        klog_putc(*str++);
    }
}

// 从日志缓冲区读取字符
int klog_getc(void) {
    if (!klog_initialized || klog_is_empty()) {
        return -1;
    }
    
    char c = klog_buffer[klog_tail];
    klog_tail = (klog_tail + 1) % KMSG_BUFFER_SIZE;
    return c;
}

// 刷新日志缓冲区到TTY
void klog_flush(void) {
    if (!kernel_tty_session) return;
    
    while (!klog_is_empty()) {
        int c = klog_getc();
        if (c >= 0) {
            char ch = (char)c;
            kernel_tty_session->ops.write(kernel_tty_session, &ch, 1);
        }
    }
    
    // 刷新TTY输出
    kernel_tty_session->ops.flush(kernel_tty_session);
}

// 格式化输出到TTY
static void tty_vprintf(const char *fmt, va_list args) {
    if (!kernel_tty_session) return;
    
    char buffer[1024];
    int len = 0;
    
    // 简单的格式化实现
    const char *p = fmt;
    while (*p && len < 1023) {
        if (*p == '%') {
            p++;
            switch (*p) {
            case 'd': {
                int val = va_arg(args, int);
                if (val < 0) {
                    buffer[len++] = '-';
                    val = -val;
                }
                if (val == 0) {
                    buffer[len++] = '0';
                } else {
                    char temp[32];
                    int i = 0;
                    while (val > 0) {
                        temp[i++] = '0' + (val % 10);
                        val /= 10;
                    }
                    while (i > 0) {
                        buffer[len++] = temp[--i];
                    }
                }
                break;
            }
            case 'x': {
                unsigned int val = va_arg(args, unsigned int);
                buffer[len++] = '0';
                buffer[len++] = 'x';
                if (val == 0) {
                    buffer[len++] = '0';
                } else {
                    char hex[] = "0123456789ABCDEF";
                    char temp[32];
                    int i = 0;
                    while (val > 0) {
                        temp[i++] = hex[val & 0xF];
                        val >>= 4;
                    }
                    while (i > 0) {
                        buffer[len++] = temp[--i];
                    }
                }
                break;
            }
            case 's': {
                const char *str = va_arg(args, const char*);
                if (str) {
                    while (*str && len < 1023) {
                        buffer[len++] = *str++;
                    }
                }
                break;
            }
            case 'c': {
                char c = va_arg(args, int);
                buffer[len++] = c;
                break;
            }
            default:
                buffer[len++] = *p;
                break;
            }
        } else {
            buffer[len++] = *p;
        }
        p++;
    }
    
    buffer[len] = '\0';
    
    // 输出到TTY
    kernel_tty_session->ops.write(kernel_tty_session, buffer, len);
    kernel_tty_session->ops.flush(kernel_tty_session);
}

// 内核输出函数
void kprint(const char *str) {
    if (!str || !kernel_tty_session) return;
    
    kernel_tty_session->ops.write(kernel_tty_session, str, shell_strlen(str));
    kernel_tty_session->ops.flush(kernel_tty_session);
}

void kprintf(const char *fmt, ...) {
    if (!fmt || !kernel_tty_session) return;
    
    va_list args;
    va_start(args, fmt);
    tty_vprintf(fmt, args);
    va_end(args);
}

void kprint_color(uint8_t fg, uint8_t bg, const char *fmt, ...) {
    if (!fmt || !kernel_tty_session) return;
    
    // 设置颜色
    char color_cmd[32];
    int len = 0;
    
    color_cmd[len++] = '\033';
    color_cmd[len++] = '[';
    
    // 前景色
    if (fg < 8) {
        color_cmd[len++] = '3';
        color_cmd[len++] = '0' + fg;
    } else {
        color_cmd[len++] = '9';
        color_cmd[len++] = '0' + (fg - 8);
    }
    
    color_cmd[len++] = ';';
    
    // 背景色
    if (bg < 8) {
        color_cmd[len++] = '4';
        color_cmd[len++] = '0' + bg;
    } else {
        color_cmd[len++] = '1';
        color_cmd[len++] = '0';
        color_cmd[len++] = '0' + (bg - 8);
    }
    
    color_cmd[len++] = 'm';
    
    // 输出颜色命令
    kernel_tty_session->ops.write(kernel_tty_session, color_cmd, len);
    
    // 输出格式化文本
    va_list args;
    va_start(args, fmt);
    tty_vprintf(fmt, args);
    va_end(args);
    
    // 重置颜色
    kernel_tty_session->ops.write(kernel_tty_session, "\033[0m", 4);
    kernel_tty_session->ops.flush(kernel_tty_session);
}

// 日志级别输出函数
void kdebug(const char *fmt, ...) {
    if (!kernel_tty_session) return;
    
    // 输出日志级别标识
    kernel_tty_session->ops.write(kernel_tty_session, log_level_colors[LOG_LEVEL_DEBUG], 5);
    kernel_tty_session->ops.write(kernel_tty_session, "[", 1);
    kernel_tty_session->ops.write(kernel_tty_session, log_level_names[LOG_LEVEL_DEBUG], 5);
    kernel_tty_session->ops.write(kernel_tty_session, "]", 1);
    kernel_tty_session->ops.write(kernel_tty_session, "\033[0m", 4);
    kernel_tty_session->ops.write(kernel_tty_session, " ", 1);
    
    // 输出格式化消息
    va_list args;
    va_start(args, fmt);
    tty_vprintf(fmt, args);
    va_end(args);
    
    kernel_tty_session->ops.write(kernel_tty_session, "\n", 1);
    kernel_tty_session->ops.flush(kernel_tty_session);
}

void kinfo(const char *fmt, ...) {
    if (!kernel_tty_session) return;
    
    kernel_tty_session->ops.write(kernel_tty_session, log_level_colors[LOG_LEVEL_INFO], 5);
    kernel_tty_session->ops.write(kernel_tty_session, "[", 1);
    kernel_tty_session->ops.write(kernel_tty_session, log_level_names[LOG_LEVEL_INFO], 5);
    kernel_tty_session->ops.write(kernel_tty_session, "]", 1);
    kernel_tty_session->ops.write(kernel_tty_session, "\033[0m", 4);
    kernel_tty_session->ops.write(kernel_tty_session, " ", 1);
    
    va_list args;
    va_start(args, fmt);
    tty_vprintf(fmt, args);
    va_end(args);
    
    kernel_tty_session->ops.write(kernel_tty_session, "\n", 1);
    kernel_tty_session->ops.flush(kernel_tty_session);
}

void kwarn(const char *fmt, ...) {
    if (!kernel_tty_session) return;
    
    kernel_tty_session->ops.write(kernel_tty_session, log_level_colors[LOG_LEVEL_WARN], 5);
    kernel_tty_session->ops.write(kernel_tty_session, "[", 1);
    kernel_tty_session->ops.write(kernel_tty_session, log_level_names[LOG_LEVEL_WARN], 5);
    kernel_tty_session->ops.write(kernel_tty_session, "]", 1);
    kernel_tty_session->ops.write(kernel_tty_session, "\033[0m", 4);
    kernel_tty_session->ops.write(kernel_tty_session, " ", 1);
    
    va_list args;
    va_start(args, fmt);
    tty_vprintf(fmt, args);
    va_end(args);
    
    kernel_tty_session->ops.write(kernel_tty_session, "\n", 1);
    kernel_tty_session->ops.flush(kernel_tty_session);
}

void kerror(const char *fmt, ...) {
    if (!kernel_tty_session) return;
    
    kernel_tty_session->ops.write(kernel_tty_session, log_level_colors[LOG_LEVEL_ERROR], 5);
    kernel_tty_session->ops.write(kernel_tty_session, "[", 1);
    kernel_tty_session->ops.write(kernel_tty_session, log_level_names[LOG_LEVEL_ERROR], 5);
    kernel_tty_session->ops.write(kernel_tty_session, "]", 1);
    kernel_tty_session->ops.write(kernel_tty_session, "\033[0m", 4);
    kernel_tty_session->ops.write(kernel_tty_session, " ", 1);
    
    va_list args;
    va_start(args, fmt);
    tty_vprintf(fmt, args);
    va_end(args);
    
    kernel_tty_session->ops.write(kernel_tty_session, "\n", 1);
    kernel_tty_session->ops.flush(kernel_tty_session);
}

void kfatal(const char *fmt, ...) {
    if (!kernel_tty_session) return;
    
    kernel_tty_session->ops.write(kernel_tty_session, log_level_colors[LOG_LEVEL_FATAL], 5);
    kernel_tty_session->ops.write(kernel_tty_session, "[", 1);
    kernel_tty_session->ops.write(kernel_tty_session, log_level_names[LOG_LEVEL_FATAL], 5);
    kernel_tty_session->ops.write(kernel_tty_session, "]", 1);
    kernel_tty_session->ops.write(kernel_tty_session, "\033[0m", 4);
    kernel_tty_session->ops.write(kernel_tty_session, " ", 1);
    
    va_list args;
    va_start(args, fmt);
    tty_vprintf(fmt, args);
    va_end(args);
    
    kernel_tty_session->ops.write(kernel_tty_session, "\n", 1);
    kernel_tty_session->ops.flush(kernel_tty_session);
}
