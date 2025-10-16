// Boruix OS 键盘驱动实现
// 处理PS/2键盘输入

#include "drivers/keyboard.h"
#include "drivers/display.h"
#include "kernel/kernel.h"

// 全局键盘状态
static keyboard_state_t keyboard_state;

// 扫描码到ASCII码的映射表（无Shift）
static const unsigned char scancode_to_ascii[128] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,   // 0x00-0x0F
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0,   0,   'a', 's', // 0x10-0x1F
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', 0,   0,   0,   'z', 'x', 'c', 'v', // 0x20-0x2F
    'b', 'n', 'm', ',', '.', '/', 0,   0,   0,   ' ', 0,   0,   0,   0,   0,   0,   // 0x30-0x3F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x40-0x4F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x50-0x5F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x60-0x6F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0    // 0x70-0x7F
};

// 扫描码到ASCII码的映射表（有Shift）
static const unsigned char scancode_to_ascii_shift[128] = {
    0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,   // 0x00-0x0F
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,   0,   'A', 'S', // 0x10-0x1F
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', 0,   0,   0,   'Z', 'X', 'C', 'V', // 0x20-0x2F
    'B', 'N', 'M', '<', '>', '?', 0,   0,   0,   ' ', 0,   0,   0,   0,   0,   0,   // 0x30-0x3F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x40-0x4F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x50-0x5F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x60-0x6F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0    // 0x70-0x7F
};

// 简单的端口输入函数
static unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

// 初始化键盘
void keyboard_init(void) {
    // 初始化键盘状态
    keyboard_state.head = 0;
    keyboard_state.tail = 0;
    keyboard_state.count = 0;
    keyboard_state.shift_pressed = 0;
    keyboard_state.ctrl_pressed = 0;
    keyboard_state.alt_pressed = 0;
    keyboard_state.caps_lock = 0;
    
    // 清空缓冲区
    for (int i = 0; i < KEYBOARD_BUFFER_SIZE; i++) {
        keyboard_state.buffer[i] = 0;
    }
}

// 读取键盘扫描码（轮询模式，非阻塞）
unsigned char keyboard_read_scancode(void) {
    // 检查数据是否可用
    if (!(inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT_BUFFER_FULL)) {
        return 0; // 没有数据
    }
    
    // 读取扫描码
    unsigned char scancode = inb(KEYBOARD_DATA_PORT);
    
    // 处理特殊键
    switch (scancode) {
        case KEY_SHIFT_LEFT:
        case KEY_SHIFT_RIGHT:
            keyboard_state.shift_pressed = 1;
            return 0; // 不返回字符
        case KEY_SHIFT_LEFT | 0x80:
        case KEY_SHIFT_RIGHT | 0x80:
            keyboard_state.shift_pressed = 0;
            return 0; // 不返回字符
        case KEY_CTRL:
            keyboard_state.ctrl_pressed = 1;
            return 0;
        case KEY_CTRL | 0x80:
            keyboard_state.ctrl_pressed = 0;
            return 0;
        case KEY_ALT:
            keyboard_state.alt_pressed = 1;
            return 0;
        case KEY_ALT | 0x80:
            keyboard_state.alt_pressed = 0;
            return 0;
        case KEY_CAPS_LOCK:
            keyboard_state.caps_lock = !keyboard_state.caps_lock;
            return 0;
        default:
            return scancode;
    }
}

// 将扫描码转换为ASCII码
unsigned char keyboard_scancode_to_ascii(unsigned char scancode) {
    // 检查是否按下（最高位为0）
    if (scancode & 0x80) {
        return 0; // 释放键
    }
    
    // 检查扫描码范围
    if (scancode >= 128) {
        return 0;
    }
    
    // 根据Shift状态选择映射表
    if (keyboard_state.shift_pressed || keyboard_state.caps_lock) {
        return scancode_to_ascii_shift[scancode];
    } else {
        return scancode_to_ascii[scancode];
    }
}

// 键盘中断处理程序
void keyboard_interrupt_handler(void) {
    unsigned char scancode = keyboard_read_scancode();
    
    // 处理特殊键
    switch (scancode) {
        case KEY_SHIFT_LEFT:
        case KEY_SHIFT_RIGHT:
            keyboard_state.shift_pressed = 1;
            break;
        case KEY_SHIFT_LEFT | 0x80:
        case KEY_SHIFT_RIGHT | 0x80:
            keyboard_state.shift_pressed = 0;
            break;
        case KEY_CTRL:
            keyboard_state.ctrl_pressed = 1;
            break;
        case KEY_CTRL | 0x80:
            keyboard_state.ctrl_pressed = 0;
            break;
        case KEY_ALT:
            keyboard_state.alt_pressed = 1;
            break;
        case KEY_ALT | 0x80:
            keyboard_state.alt_pressed = 0;
            break;
        case KEY_CAPS_LOCK:
            keyboard_state.caps_lock = !keyboard_state.caps_lock;
            break;
        default:
            // 处理普通字符
            unsigned char ascii = keyboard_scancode_to_ascii(scancode);
            if (ascii != 0) {
                // 添加到缓冲区
                if (keyboard_state.count < KEYBOARD_BUFFER_SIZE) {
                    keyboard_state.buffer[keyboard_state.tail] = ascii;
                    keyboard_state.tail = (keyboard_state.tail + 1) % KEYBOARD_BUFFER_SIZE;
                    keyboard_state.count++;
                }
            }
            break;
    }
}

// 获取一个字符（非阻塞）
unsigned char keyboard_get_char(void) {
    if (keyboard_state.count == 0) {
        return 0; // 没有字符
    }
    
    unsigned char ch = keyboard_state.buffer[keyboard_state.head];
    keyboard_state.head = (keyboard_state.head + 1) % KEYBOARD_BUFFER_SIZE;
    keyboard_state.count--;
    
    return ch;
}

// 检查是否有字符可用
int keyboard_has_char(void) {
    return keyboard_state.count > 0;
}

// 清空键盘缓冲区
void keyboard_clear_buffer(void) {
    keyboard_state.head = 0;
    keyboard_state.tail = 0;
    keyboard_state.count = 0;
}
