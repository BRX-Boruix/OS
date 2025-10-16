// Boruix OS 键盘驱动头文件
// 处理PS/2键盘输入

#ifndef BORUIX_KEYBOARD_H
#define BORUIX_KEYBOARD_H

#include "kernel/types.h"

// 键盘端口定义
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

// 键盘状态位
#define KEYBOARD_STATUS_OUTPUT_BUFFER_FULL 0x01
#define KEYBOARD_STATUS_INPUT_BUFFER_FULL 0x02

// 键盘缓冲区大小
#define KEYBOARD_BUFFER_SIZE 256

// 特殊键码定义
#define KEY_ENTER 0x1C
#define KEY_BACKSPACE 0x0E
#define KEY_TAB 0x0F
#define KEY_ESC 0x01
#define KEY_CTRL 0x1D
#define KEY_SHIFT_LEFT 0x2A
#define KEY_SHIFT_RIGHT 0x36
#define KEY_ALT 0x38
#define KEY_CAPS_LOCK 0x3A
#define KEY_F1 0x3B
#define KEY_F2 0x3C
#define KEY_F3 0x3D
#define KEY_F4 0x3E
#define KEY_F5 0x3F
#define KEY_F6 0x40
#define KEY_F7 0x41
#define KEY_F8 0x42
#define KEY_F9 0x43
#define KEY_F10 0x44
#define KEY_F11 0x57
#define KEY_F12 0x58

// 键盘状态
typedef struct {
    uint8_t buffer[KEYBOARD_BUFFER_SIZE];
    int head;
    int tail;
    int count;
    uint8_t shift_pressed;
    uint8_t ctrl_pressed;
    uint8_t alt_pressed;
    uint8_t caps_lock;
} keyboard_state_t;

// 键盘函数声明
void keyboard_init(void);
void keyboard_interrupt_handler(void);
uint8_t keyboard_read_scancode(void);
uint8_t keyboard_scancode_to_ascii(uint8_t scancode);
uint8_t keyboard_get_char(void);
int keyboard_has_char(void);
void keyboard_clear_buffer(void);

#endif // BORUIX_KEYBOARD_H
