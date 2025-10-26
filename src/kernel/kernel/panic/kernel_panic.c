// Boruix OS 内核panic处理
// 用于Rust panic handler调用

#include "kernel/types.h"

// 触发双重错误（最高级别的内核崩溃）
// 这个函数永不返回
void trigger_double_fault(void) {
    // 禁用中断
    __asm__ volatile("cli");
    
    // 方法：破坏栈然后触发异常，导致双重错误
    // 1. 将栈指针设置为无效地址
    // 2. 触发一般保护错误
    // 3. 由于栈无效，处理第一个异常时会再次出错，触发双重错误
    __asm__ volatile(
        "mov $0xDEADBEEF, %%rsp\n"  // 设置无效的栈指针
        "int $0x0D\n"                // 触发一般保护错误（GPF）
        : : : "memory"
    );
    
    // 如果双重错误处理失败，会到达这里（不应该）
    // 进入死循环
    while(1) {
        __asm__ volatile("cli; pause");
    }
}

// 触发普通内核崩溃（SHD PAGE级别）
// 这个函数永不返回
void trigger_kernel_crash(void) {
    // 禁用中断
    __asm__ volatile("cli");
    
    // 触发除零异常
    volatile int zero = 0;
    volatile int result = 1 / zero;
    (void)result;
    
    // 如果除零没有触发，访问无效内存
    volatile int* invalid_ptr = (int*)0xFFFFFFFFFFFFFFFF;
    volatile int value = *invalid_ptr;
    (void)value;
    
    // 如果以上都失败，进入死循环
    while(1) {
        __asm__ volatile("cli; pause");
    }
}
