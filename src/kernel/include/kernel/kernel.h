// Boruix OS 内核主头文件
// 内核入口点和主要逻辑（仅x86_64）

#ifndef BORUIX_KERNEL_H
#define BORUIX_KERNEL_H

#include "kernel/types.h"
#include "arch/x86_64.h"

// 内核主函数（x86_64）
void kernel_main(uint32_t magic, uint64_t multiboot_info);

#endif // BORUIX_KERNEL_H
