// Boruix OS Shell - crash命令实现
// 手动触发系统崩溃

#include "crash.h"
#include "kernel/shell.h"
#include "drivers/display.h"

// 手动触发系统崩溃
void cmd_crash(int argc, char* argv[]) {
    (void)argc; (void)argv;  // 避免未使用参数警告
    
    print_string("System crash initiated by user command.\n");
    print_string("Triggering manual system crash...\n");
    
    // 禁用中断
    __asm__ volatile("cli");
    
    // 方法1: 触发除零异常（最简单）
    print_string("Triggering division by zero exception...\n");
    volatile int zero = 0;
    volatile int result = 1 / zero;  // 这会触发除零异常
    (void)result;  // 避免未使用变量警告
    
    // 如果除零异常没有触发崩溃，尝试其他方法
    print_string("Division by zero failed, trying invalid memory access...\n");
    
    // 方法2: 访问无效内存地址
    volatile int* invalid_ptr = (int*)0xFFFFFFFFFFFFFFFF;
    volatile int value = *invalid_ptr;  // 这会触发页面错误
    (void)value;  // 避免未使用变量警告
    
    // 方法3: 如果以上都失败，进入无限循环
    print_string("All crash methods failed, entering infinite loop...\n");
    while(1) {
        __asm__ volatile("hlt");
    }
}
