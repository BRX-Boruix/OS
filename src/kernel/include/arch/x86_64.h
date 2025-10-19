// Boruix OS 架构相关定义
// x86_64架构特定的常量和定义

#ifndef BORUIX_ARCH_X86_64_H
#define BORUIX_ARCH_X86_64_H

#include "kernel/types.h"

// Multiboot2头定义 (x86_64使用multiboot2)
#define MULTIBOOT2_HEADER_MAGIC 0xe85250d6
#define MULTIBOOT2_ARCHITECTURE_I386 0
#define MULTIBOOT2_HEADER_LENGTH 16
#define MULTIBOOT2_CHECKSUM -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_ARCHITECTURE_I386 + MULTIBOOT2_HEADER_LENGTH)

// CMOS端口定义
#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

// CMOS寄存器定义
#define CMOS_SECOND 0x00
#define CMOS_MINUTE 0x02
#define CMOS_HOUR 0x04
#define CMOS_DAY 0x07
#define CMOS_MONTH 0x08
#define CMOS_YEAR 0x09

// 中断向量定义
#define IRQ0_TIMER 0x20
#define IRQ1_KEYBOARD 0x21

// 中断描述符表相关
#define IDT_SIZE 256
#define IDT_BASE 0x0000000000000000ULL

// 中断门类型 (x86_64)
#define IDT_INTERRUPT_GATE 0x8E
#define IDT_TRAP_GATE 0x8F

// x86_64中断描述符结构 (64位)
typedef struct {
    uint16_t offset_low;    // 偏移地址低16位
    uint16_t selector;      // 段选择子
    uint8_t ist;           // 中断栈表索引
    uint8_t type;          // 类型和属性
    uint16_t offset_mid;    // 偏移地址中16位
    uint32_t offset_high;   // 偏移地址高32位
    uint32_t reserved;      // 保留字段
} __attribute__((packed)) idt_entry_t;

// IDT指针结构 (64位)
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

// 页表相关定义 (x86_64特有)
#define PAGE_SIZE 4096
#define PAGE_PRESENT 0x1
#define PAGE_WRITABLE 0x2
#define PAGE_USER 0x4

// 64位页表项
typedef struct {
    uint64_t present : 1;
    uint64_t writable : 1;
    uint64_t user : 1;
    uint64_t write_through : 1;
    uint64_t cache_disable : 1;
    uint64_t accessed : 1;
    uint64_t dirty : 1;
    uint64_t page_size : 1;
    uint64_t global : 1;
    uint64_t available : 3;
    uint64_t address : 40;
    uint64_t available2 : 11;
    uint64_t no_execute : 1;
} __attribute__((packed)) page_entry_t;

#endif // BORUIX_ARCH_X86_64_H
