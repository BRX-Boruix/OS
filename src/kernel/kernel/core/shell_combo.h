// Boruix OS Shell - 组合键处理
// 处理各种键盘组合键功能

#ifndef SHELL_COMBO_H
#define SHELL_COMBO_H

#include "kernel/types.h"

// 显示控制字符（如^C, ^V等）
void shell_display_control_char(uint8_t key);

// 通用组合键处理函数
void shell_handle_combo_sequence(uint8_t* sequence, int length, uint8_t modifiers);

#endif // SHELL_COMBO_H
