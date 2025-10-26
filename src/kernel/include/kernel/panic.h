// Boruix OS 内核panic处理头文件

#ifndef BORUIX_PANIC_H
#define BORUIX_PANIC_H

// 触发双重错误（最高级别崩溃，永不返回）
void trigger_double_fault(void) __attribute__((noreturn));

// 触发普通内核崩溃（SHD PAGE级别，永不返回）
void trigger_kernel_crash(void) __attribute__((noreturn));

#endif // BORUIX_PANIC_H
