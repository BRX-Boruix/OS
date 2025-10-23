#ifndef TSS_H
#define TSS_H

#include "kernel/types.h"

// TSS初始化
void tss_init(void);

// 获取TSS基址（用于调试）
uint64_t tss_get_base(void);

// 获取双重错误栈地址（用于调试）
uint64_t tss_get_double_fault_stack(void);

#endif // TSS_H

