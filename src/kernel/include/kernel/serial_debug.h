// 串口调试输出头文件

#ifndef KERNEL_SERIAL_DEBUG_H
#define KERNEL_SERIAL_DEBUG_H

#include "kernel/types.h"

// 初始化串口
void serial_debug_init(void);

// 发送一个字符到串口
void serial_putchar(char c);

// 发送字符串到串口
void serial_puts(const char* str);

// 发送十六进制数到串口
void serial_put_hex(uint64_t value);

// 发送十进制数到串口
void serial_put_dec(uint64_t value);

// 便捷宏
#define SERIAL_DEBUG(msg) serial_puts("[DEBUG] " msg "\n")
#define SERIAL_INFO(msg) serial_puts("[INFO] " msg "\n")
#define SERIAL_ERROR(msg) serial_puts("[ERROR] " msg "\n")

#endif // KERNEL_SERIAL_DEBUG_H

