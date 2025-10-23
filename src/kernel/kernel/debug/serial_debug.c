// 串口调试输出
// 用于在QEMU中输出调试信息

#include "kernel/types.h"

#define SERIAL_PORT_COM1 0x3F8

// I/O端口操作
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// 初始化串口
void serial_debug_init(void) {
    outb(SERIAL_PORT_COM1 + 1, 0x00);    // 禁用中断
    outb(SERIAL_PORT_COM1 + 3, 0x80);    // 启用DLAB（设置波特率）
    outb(SERIAL_PORT_COM1 + 0, 0x03);    // 波特率低字节 (38400 baud)
    outb(SERIAL_PORT_COM1 + 1, 0x00);    // 波特率高字节
    outb(SERIAL_PORT_COM1 + 3, 0x03);    // 8位，无奇偶校验，1停止位
    outb(SERIAL_PORT_COM1 + 2, 0xC7);    // 启用FIFO，清除，14字节阈值
    outb(SERIAL_PORT_COM1 + 4, 0x0B);    // IRQs启用，RTS/DSR设置
}

// 检查串口是否可以发送
static int serial_is_transmit_empty(void) {
    return inb(SERIAL_PORT_COM1 + 5) & 0x20;
}

// 发送一个字符到串口
void serial_putchar(char c) {
    while (serial_is_transmit_empty() == 0);
    outb(SERIAL_PORT_COM1, c);
}

// 发送字符串到串口
void serial_puts(const char* str) {
    if (!str) return;
    while (*str) {
        if (*str == '\n') {
            serial_putchar('\r');
        }
        serial_putchar(*str);
        str++;
    }
}

// 发送十六进制数到串口
void serial_put_hex(uint64_t value) {
    const char hex_chars[] = "0123456789ABCDEF";
    serial_puts("0x");
    
    int started = 0;
    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        if (nibble != 0 || started || i == 0) {
            serial_putchar(hex_chars[nibble]);
            started = 1;
        }
    }
}

// 发送十进制数到串口
void serial_put_dec(uint64_t value) {
    if (value == 0) {
        serial_putchar('0');
        return;
    }
    
    char buffer[20];
    int i = 0;
    
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    while (i > 0) {
        serial_putchar(buffer[--i]);
    }
}

