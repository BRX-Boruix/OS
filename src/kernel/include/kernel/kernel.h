// Boruix OS 内核主头文件
// 内核入口点和主要逻辑

#ifndef BORUIX_KERNEL_H
#define BORUIX_KERNEL_H

#include "kernel/types.h"

// 根据架构包含相应的头文件
#ifdef __x86_64__
#include "arch/x86_64.h"
// 64位内核主函数
void kernel_main(uint32_t magic, uint64_t multiboot_info);
#else
#include "arch/i386.h"
// 32位内核主函数
void kernel_main(uint32_t magic, uint32_t multiboot_info);
#endif

#endif // BORUIX_KERNEL_H