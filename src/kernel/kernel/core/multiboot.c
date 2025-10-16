// Boruix OS Multiboot头实现
// 必须在文件开头，用于GRUB识别

// Multiboot头现在在汇编文件中定义
#ifndef __x86_64__
#include "kernel/multiboot.h"
#endif

// Multiboot头已经在头文件中定义
