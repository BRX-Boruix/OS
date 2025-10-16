// Boruix OS 内核主头文件
// 内核入口点和主要逻辑

#ifndef BORUIX_KERNEL_H
#define BORUIX_KERNEL_H

#include "kernel/types.h"
#include "arch/i386.h"

// 函数声明
void kernel_main(uint32_t magic, uint32_t multiboot_info);

#endif // BORUIX_KERNEL_H