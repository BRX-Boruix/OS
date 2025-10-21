// Boruix OS 键盘驱动实现
// 处理PS/2键盘输入，支持通用组合键系统

#include "drivers/keyboard.h"
#include "drivers/display.h"
#include "kernel/kernel.h"

// 全局键盘状态
static keyboard_state_t keyboard_state;

// 全局时间戳计数器（简单实现）
static uint32_t global_timestamp = 0;

// 扩展扫描码状态
static int extended_scancode = 0;

// 函数声明
static uint32_t get_timestamp(void);
static void increment_timestamp(void);
static void add_combo_event(combo_event_type_t type, uint8_t scancode, uint8_t ascii);
static void process_key_event(uint8_t scancode);
static void reset_combo_sequence(void);
static int is_modifier_key(uint8_t scancode);
static uint8_t get_modifier_state(void);

// 扫描码到ASCII码的映射表（无Shift）
static const unsigned char scancode_to_ascii[128] = {
    0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', 0,   // 0x00-0x0F
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,   'a', 's', // 0x10-0x1F
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', 0,   0,   0,   'z', 'x', 'c', 'v', // 0x20-0x2F
    'b', 'n', 'm', ',', '.', '/', 0,   0,   0,   ' ', 0,   0,   0,   0,   0,   0,   // 0x30-0x3F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x40-0x4F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x50-0x5F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 0x60-0x6F
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0    // 0x70-0x7F
};

// 扫描码到ASCII码的映射表（有Shift）
static const unsigned char scancode_to_ascii_shift[128] = {
    0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', 0,   // 0x00-0x0F
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,   'A', 'S', // 0x10-0x1F
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
    
    // 初始化通用组合键系统
    keyboard_state.combo_state.sequence_length = 0;
    keyboard_state.combo_state.last_event_time = 0;
    keyboard_state.combo_state.modifier_state = 0;
    keyboard_state.combo_state.is_active = 0;
    
    keyboard_state.event_head = 0;
    keyboard_state.event_tail = 0;
    keyboard_state.event_count = 0;
    
    // 清空缓冲区
    for (int i = 0; i < KEYBOARD_BUFFER_SIZE; i++) {
        keyboard_state.buffer[i] = 0;
        keyboard_state.event_buffer[i].type = COMBO_EVENT_NONE;
        keyboard_state.event_buffer[i].scancode = 0;
        keyboard_state.event_buffer[i].ascii = 0;
        keyboard_state.event_buffer[i].timestamp = 0;
    }
    
    // 清空组合键序列
    for (int i = 0; i < MAX_COMBO_SEQUENCE; i++) {
        keyboard_state.combo_state.sequence[i] = 0;
    }
    
    global_timestamp = 0;
}

// 键盘中断处理程序
void keyboard_irq_handler(void) {
    // 检查数据是否可用
    if (!(inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT_BUFFER_FULL)) {
        return; // 没有数据
    }
    
    // 读取扫描码
    unsigned char scancode = inb(KEYBOARD_DATA_PORT);
    
    // 处理键盘事件
    process_key_event(scancode);
    
    increment_timestamp(); // 增加时间戳
}

// 读取键盘扫描码（轮询模式，非阻塞）
unsigned char keyboard_read_scancode(void) {
    // 检查数据是否可用
    if (!(inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT_BUFFER_FULL)) {
        increment_timestamp(); // 增加时间戳
        return 0; // 没有数据
    }
    
    // 读取扫描码
    unsigned char scancode = inb(KEYBOARD_DATA_PORT);
    
    // 处理键盘事件
    process_key_event(scancode);
    
    increment_timestamp(); // 增加时间戳
    return scancode;
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

// 键盘中断处理程序（使用新的事件系统）
void keyboard_interrupt_handler(void) {
    keyboard_read_scancode(); // 这会自动处理所有键盘事件
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
    keyboard_state.event_head = 0;
    keyboard_state.event_tail = 0;
    keyboard_state.event_count = 0;
    reset_combo_sequence();
}

// === 通用组合键系统实现 ===

// 获取时间戳
static uint32_t get_timestamp(void) {
    return global_timestamp;
}

// 增加时间戳
static void increment_timestamp(void) {
    global_timestamp++;
}

// 添加组合键事件到缓冲区
static void add_combo_event(combo_event_type_t type, uint8_t scancode, uint8_t ascii) {
    if (keyboard_state.event_count >= KEYBOARD_BUFFER_SIZE) {
        return; // 缓冲区满
    }
    
    combo_event_t* event = &keyboard_state.event_buffer[keyboard_state.event_tail];
    event->type = type;
    event->scancode = scancode;
    event->ascii = ascii;
    event->timestamp = get_timestamp();
    
    keyboard_state.event_tail = (keyboard_state.event_tail + 1) % KEYBOARD_BUFFER_SIZE;
    keyboard_state.event_count++;
}

// 处理键盘事件
static void process_key_event(uint8_t scancode) {
    // 检查是否是扩展扫描码前缀
    if (scancode == 0xE0) {
        extended_scancode = 1;
        return;
    }
    
    uint8_t is_key_up = (scancode & 0x80) != 0;
    uint8_t key_code = scancode & 0x7F;
    
    // 如果是扩展扫描码，调整键码
    if (extended_scancode) {
        // 扩展扫描码的Page Up/Page Down等键
        if (key_code == 0x49) key_code = KEY_PAGE_UP;      // Page Up
        else if (key_code == 0x51) key_code = KEY_PAGE_DOWN; // Page Down
        else if (key_code == 0x48) key_code = KEY_UP_ARROW;  // Up Arrow
        else if (key_code == 0x50) key_code = KEY_DOWN_ARROW; // Down Arrow
        else if (key_code == 0x4B) key_code = KEY_LEFT_ARROW; // Left Arrow
        else if (key_code == 0x4D) key_code = KEY_RIGHT_ARROW; // Right Arrow
        else if (key_code == 0x47) key_code = KEY_HOME;      // Home
        else if (key_code == 0x4F) key_code = KEY_END;       // End
        else if (key_code == 0x52) key_code = KEY_INSERT;     // Insert
        else if (key_code == 0x53) key_code = KEY_DELETE;     // Delete
        else {
            // 不是特殊键，忽略这个扫描码
            extended_scancode = 0;
            return;
        }
        
        extended_scancode = 0; // 重置扩展扫描码状态
    }
    
    if (is_key_up) {
        // 键释放事件
        add_combo_event(COMBO_EVENT_KEY_UP, key_code, 0);
        
        // 更新修饰键状态
        switch (key_code) {
            case KEY_SHIFT_LEFT:
            case KEY_SHIFT_RIGHT:
                keyboard_state.shift_pressed = 0;
                add_combo_event(COMBO_EVENT_MODIFIER_CHANGE, key_code, 0);
                // 如果所有修饰键都释放了，重置组合键状态
                if (!keyboard_state.ctrl_pressed && !keyboard_state.alt_pressed) {
                    reset_combo_sequence();
                }
                break;
            case KEY_CTRL:
                keyboard_state.ctrl_pressed = 0;
                add_combo_event(COMBO_EVENT_MODIFIER_CHANGE, key_code, 0);
                // 如果所有修饰键都释放了，重置组合键状态
                if (!keyboard_state.shift_pressed && !keyboard_state.alt_pressed) {
                    reset_combo_sequence();
                }
                break;
            case KEY_ALT:
                keyboard_state.alt_pressed = 0;
                add_combo_event(COMBO_EVENT_MODIFIER_CHANGE, key_code, 0);
                // 如果所有修饰键都释放了，重置组合键状态
                if (!keyboard_state.shift_pressed && !keyboard_state.ctrl_pressed) {
                    reset_combo_sequence();
                }
                break;
        }
    } else {
        // 键按下事件
        add_combo_event(COMBO_EVENT_KEY_DOWN, key_code, 0);
        
        // 更新修饰键状态
        switch (key_code) {
            case KEY_SHIFT_LEFT:
            case KEY_SHIFT_RIGHT:
                keyboard_state.shift_pressed = 1;
                add_combo_event(COMBO_EVENT_MODIFIER_CHANGE, key_code, 0);
                break;
            case KEY_CTRL:
                keyboard_state.ctrl_pressed = 1;
                add_combo_event(COMBO_EVENT_MODIFIER_CHANGE, key_code, 0);
                break;
            case KEY_ALT:
                keyboard_state.alt_pressed = 1;
                add_combo_event(COMBO_EVENT_MODIFIER_CHANGE, key_code, 0);
                break;
            case KEY_CAPS_LOCK:
                keyboard_state.caps_lock = !keyboard_state.caps_lock;
                break;
            default:
                // 处理特殊键（方向键、翻页键等）
                if (key_code == KEY_PAGE_UP || key_code == KEY_PAGE_DOWN || 
                    key_code == KEY_UP_ARROW || key_code == KEY_DOWN_ARROW ||
                    key_code == KEY_LEFT_ARROW || key_code == KEY_RIGHT_ARROW ||
                    key_code == KEY_HOME || key_code == KEY_END ||
                    key_code == KEY_INSERT || key_code == KEY_DELETE) {
                    // 特殊键不产生ASCII字符，但需要特殊处理
                    // 这里我们使用特殊的ASCII码来表示这些键
                    unsigned char special_ascii = 0;
                    switch (key_code) {
                        case KEY_PAGE_UP: special_ascii = 0x02; break;      // Ctrl+B
                        case KEY_PAGE_DOWN: special_ascii = 0x01; break;    // Ctrl+A
                        case KEY_UP_ARROW: special_ascii = 0x05; break;      // Ctrl+E
                        case KEY_DOWN_ARROW: special_ascii = 0x06; break;   // Ctrl+F
                        case KEY_LEFT_ARROW: special_ascii = 0x03; break;    // Ctrl+C
                        case KEY_RIGHT_ARROW: special_ascii = 0x04; break;   // Ctrl+D
                        case KEY_HOME: special_ascii = 0x07; break;         // Ctrl+G
                        case KEY_END: special_ascii = 0x08; break;          // Ctrl+H
                        case KEY_INSERT: special_ascii = 0x09; break;        // Ctrl+I
                        case KEY_DELETE: special_ascii = 0x0A; break;         // Ctrl+J
                    }
                    
                    if (special_ascii != 0) {
                        // 添加到普通字符缓冲区
                        if (keyboard_state.count < KEYBOARD_BUFFER_SIZE) {
                            keyboard_state.buffer[keyboard_state.tail] = special_ascii;
                            keyboard_state.tail = (keyboard_state.tail + 1) % KEYBOARD_BUFFER_SIZE;
                            keyboard_state.count++;
                        }
                    }
                } else {
                    // 处理普通字符
                    unsigned char ascii = keyboard_scancode_to_ascii(scancode);
                    if (ascii != 0) {
                        // 添加到普通字符缓冲区
                        if (keyboard_state.count < KEYBOARD_BUFFER_SIZE) {
                            keyboard_state.buffer[keyboard_state.tail] = ascii;
                            keyboard_state.tail = (keyboard_state.tail + 1) % KEYBOARD_BUFFER_SIZE;
                            keyboard_state.count++;
                        }
                    }
                }
                
                // 只有在按下修饰键时才添加到组合键序列
                if (keyboard_state.ctrl_pressed || keyboard_state.shift_pressed || keyboard_state.alt_pressed) {
                    if (keyboard_state.combo_state.sequence_length < MAX_COMBO_SEQUENCE) {
                        keyboard_state.combo_state.sequence[keyboard_state.combo_state.sequence_length] = key_code;
                        keyboard_state.combo_state.sequence_length++;
                        keyboard_state.combo_state.last_event_time = get_timestamp();
                        keyboard_state.combo_state.modifier_state = get_modifier_state();
                        keyboard_state.combo_state.is_active = 1;
                    }
                }
                break;
        }
    }
}

// 重置组合键序列
static void reset_combo_sequence(void) {
    keyboard_state.combo_state.sequence_length = 0;
    keyboard_state.combo_state.last_event_time = 0;
    keyboard_state.combo_state.modifier_state = 0;
    keyboard_state.combo_state.is_active = 0;
    
    for (int i = 0; i < MAX_COMBO_SEQUENCE; i++) {
        keyboard_state.combo_state.sequence[i] = 0;
    }
}

// 检查是否为修饰键（保留以备将来使用）
static int is_modifier_key(uint8_t scancode) {
    return (scancode == KEY_SHIFT_LEFT || scancode == KEY_SHIFT_RIGHT ||
            scancode == KEY_CTRL || scancode == KEY_ALT);
}

// 获取修饰键状态
static uint8_t get_modifier_state(void) {
    uint8_t state = 0;
    if (keyboard_state.shift_pressed) state |= 0x01;
    if (keyboard_state.ctrl_pressed) state |= 0x02;
    if (keyboard_state.alt_pressed) state |= 0x04;
    if (keyboard_state.caps_lock) state |= 0x08;
    return state;
}

// === 公共API函数 ===

// 获取组合键事件
combo_event_t keyboard_get_combo_event(void) {
    if (keyboard_state.event_count == 0) {
        combo_event_t empty_event = {COMBO_EVENT_NONE, 0, 0, 0};
        return empty_event;
    }
    
    combo_event_t event = keyboard_state.event_buffer[keyboard_state.event_head];
    keyboard_state.event_head = (keyboard_state.event_head + 1) % KEYBOARD_BUFFER_SIZE;
    keyboard_state.event_count--;
    
    return event;
}

// 检查是否有组合键事件
int keyboard_has_combo_event(void) {
    return keyboard_state.event_count > 0;
}

// 处理组合键序列
void keyboard_process_combo_sequence(uint8_t* sequence, int length, uint8_t modifiers) {
    // 这个函数可以被shell调用来处理完整的组合键序列
    // 序列格式：[修饰键状态][按键1][按键2]...[按键N]
    if (length > 0 && length <= MAX_COMBO_SEQUENCE) {
        keyboard_state.combo_state.modifier_state = modifiers;
        keyboard_state.combo_state.sequence_length = length;
        keyboard_state.combo_state.last_event_time = get_timestamp();
        keyboard_state.combo_state.is_active = 1;
        
        for (int i = 0; i < length; i++) {
            keyboard_state.combo_state.sequence[i] = sequence[i];
        }
    }
}

// 检查组合键是否激活
int keyboard_is_combo_active(void) {
    return keyboard_state.combo_state.is_active;
}

// 重置组合键状态
void keyboard_reset_combo_state(void) {
    reset_combo_sequence();
}

// 获取修饰键状态
uint8_t keyboard_get_modifier_state(void) {
    return get_modifier_state();
}

// 检查修饰键状态
int keyboard_is_ctrl_pressed(void) {
    return keyboard_state.ctrl_pressed;
}

int keyboard_is_shift_pressed(void) {
    return keyboard_state.shift_pressed;
}

int keyboard_is_alt_pressed(void) {
    return keyboard_state.alt_pressed;
}
