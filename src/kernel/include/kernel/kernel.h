// Boruix OS 内核主头文件
// 内核入口点和主要逻辑（仅x86_64）

#ifndef BORUIX_KERNEL_H
#define BORUIX_KERNEL_H

#include "kernel/types.h"
#include "arch/x86_64.h"

// 内核主函数 - Limine入口点（CoolPotOS风格）
void kmain(void);

#endif // BORUIX_KERNEL_H
