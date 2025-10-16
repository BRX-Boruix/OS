// Boruix OS Multiboot2头文件
// 用于x86_64架构的GRUB2引导

#ifndef BORUIX_MULTIBOOT2_H
#define BORUIX_MULTIBOOT2_H

#include "kernel/types.h"

// Multiboot2常量
#define MULTIBOOT2_HEADER_MAGIC    0xe85250d6
#define MULTIBOOT2_ARCHITECTURE    0          // i386 (兼容x86_64)

// 标签类型
#define MULTIBOOT2_TAG_END         0
#define MULTIBOOT2_TAG_INFO_REQ    1
#define MULTIBOOT2_TAG_ADDRESS     2
#define MULTIBOOT2_TAG_ENTRY       3
#define MULTIBOOT2_TAG_CONSOLE     4
#define MULTIBOOT2_TAG_FRAMEBUFFER 5

// Multiboot2头结构（必须在文件开头）
struct multiboot2_header {
    uint32_t magic;
    uint32_t architecture;
    uint32_t header_length;
    uint32_t checksum;
} __attribute__((packed));

// 结束标签
struct multiboot2_tag_end {
    uint16_t type;
    uint16_t flags;
    uint32_t size;
} __attribute__((packed));

// 完整的multiboot2头
__attribute__((section(".multiboot")))
__attribute__((aligned(8)))
const struct {
    struct multiboot2_header header;
    struct multiboot2_tag_end end_tag;
} multiboot2_header = {
    // Multiboot2头
    {
        MULTIBOOT2_HEADER_MAGIC,
        MULTIBOOT2_ARCHITECTURE,
        24,  // 固定长度：16字节头 + 8字节结束标签
        0x17ada00e  // 预计算的校验和
    },
    // 结束标签
    {
        MULTIBOOT2_TAG_END,
        0,
        8
    }
};

// Multiboot2信息结构
struct multiboot2_info {
    uint32_t total_size;
    uint32_t reserved;
} __attribute__((packed));

#endif // BORUIX_MULTIBOOT2_H
