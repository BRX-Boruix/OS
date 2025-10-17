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

// 通用组合键系统配置
#define MAX_COMBO_LEVELS 10        // 最大组合键层级
#define MAX_COMBO_SEQUENCE 32      // 最大组合键序列长度
#define COMBO_TIMEOUT_MS 1000      // 组合键超时时间（毫秒）

// 组合键事件类型
typedef enum {
    COMBO_EVENT_NONE = 0,
    COMBO_EVENT_KEY_DOWN,
    COMBO_EVENT_KEY_UP,
    COMBO_EVENT_MODIFIER_CHANGE
} combo_event_type_t;

// 组合键事件结构
typedef struct {
    combo_event_type_t type;
    uint8_t scancode;
    uint8_t ascii;
    uint32_t timestamp;  // 简单计数器作为时间戳
} combo_event_t;

// 组合键状态
typedef struct {
    uint8_t sequence[MAX_COMBO_SEQUENCE];  // 按键序列
    int sequence_length;                   // 序列长度
    uint32_t last_event_time;              // 最后事件时间
    uint8_t modifier_state;                // 修饰键状态
    uint8_t is_active;                     // 是否正在组合键输入
} combo_state_t;

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
    
    // 通用组合键系统
    combo_state_t combo_state;
    combo_event_t event_buffer[KEYBOARD_BUFFER_SIZE];
    int event_head;
    int event_tail;
    int event_count;
} keyboard_state_t;

// 键盘函数声明
void keyboard_init(void);
void keyboard_interrupt_handler(void);
void keyboard_irq_handler(void);  // 键盘中断处理程序
uint8_t keyboard_read_scancode(void);
uint8_t keyboard_scancode_to_ascii(uint8_t scancode);
uint8_t keyboard_get_char(void);
int keyboard_has_char(void);
void keyboard_clear_buffer(void);

// 通用组合键检测函数
combo_event_t keyboard_get_combo_event(void);
int keyboard_has_combo_event(void);
void keyboard_process_combo_sequence(uint8_t* sequence, int length, uint8_t modifiers);
int keyboard_is_combo_active(void);
void keyboard_reset_combo_state(void);

// 修饰键状态查询
int keyboard_is_ctrl_pressed(void);
int keyboard_is_shift_pressed(void);
int keyboard_is_alt_pressed(void);
uint8_t keyboard_get_modifier_state(void);

#endif // BORUIX_KEYBOARD_H
