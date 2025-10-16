// Boruix OS 架构相关定义
// i386架构特定的常量和定义

#ifndef BORUIX_ARCH_I386_H
#define BORUIX_ARCH_I386_H

#include "kernel/types.h"

// Multiboot头定义
#define MULTIBOOT_HEADER_MAGIC 0x1BADB002
#define MULTIBOOT_HEADER_FLAGS 0x00000003
#define MULTIBOOT_HEADER_CHECKSUM -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

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
#define IDT_BASE 0x00000000

// 中断门类型
#define IDT_INTERRUPT_GATE 0x8E
#define IDT_TRAP_GATE 0x8F

// 中断描述符结构
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

// IDT指针结构
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

#endif // BORUIX_ARCH_I386_H
