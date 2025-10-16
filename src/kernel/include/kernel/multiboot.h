// Boruix OS Multiboot头文件
// 必须在文件开头，用于GRUB识别

#ifndef BORUIX_MULTIBOOT_H
#define BORUIX_MULTIBOOT_H

// Multiboot头结构（必须在文件开头）
__attribute__((section(".multiboot")))
__attribute__((aligned(4)))
const unsigned int multiboot_header[] = {
    0x1BADB002,  // Magic
    0x00000003,  // Flags
    0xE4524FFB   // Checksum: -(0x1BADB002 + 0x00000003)
};

#endif // BORUIX_MULTIBOOT_H
