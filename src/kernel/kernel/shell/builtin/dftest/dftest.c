// Boruix OS Shell - dftest命令实现
// 测试双重错误处理

#include "dftest.h"
#include "kernel/shell.h"
#include "drivers/display.h"

// 测试双重错误
void cmd_dftest(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    print_string("===== Double Fault Test =====\n");
    print_string("This will trigger a double fault to test the handler.\n");
    print_string("The system should display detailed fault information.\n\n");
    print_string("Triggering double fault in 3...\n");
    for(volatile int i = 0; i < 100000000; i++);
    print_string("2...\n");
    for(volatile int i = 0; i < 100000000; i++);
    print_string("1...\n");
    for(volatile int i = 0; i < 100000000; i++);
    print_string("\n");
    
    // 禁用中断
    __asm__ volatile("cli");
    
    // 方法：破坏栈然后触发异常
    // 1. 将栈指针设置为无效地址
    __asm__ volatile(
        "mov $0xDEADBEEF, %rsp\n"  // 设置无效的栈指针
        "int $0x0D\n"               // 触发一般保护错误
    );
    
    // 如果双重错误处理失败，会到达这里（不应该）
    print_string("ERROR: Should not reach here!\n");
    while(1) {
        __asm__ volatile("hlt");
    }
}

