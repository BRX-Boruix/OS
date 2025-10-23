#ifndef GDT_H
#define GDT_H

#include "kernel/types.h"

// GDT初始化
void gdt_init(void);

// 获取GDT基址
uint64_t gdt_get_base(void);

// 获取TSS选择子
uint16_t gdt_get_tss_selector(void);

#endif // GDT_H

