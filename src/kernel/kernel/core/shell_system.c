// Boruix OS Shell - 系统工具函数实现
// 提供系统级别的功能

#include "shell_system.h"
#include "drivers/display.h"

// 简单的端口输出函数
static void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a" (value), "Nd" (port));
}

// 简单的端口输入函数
static uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

// 系统重启实现
void reboot_system(void) {
    // 方法1: 通过8042键盘控制器重启（最常用）
    print_string("Attempting reboot via 8042 controller...\n");
    
    // 等待8042输入缓冲区清空
    int timeout = 100000;
    while ((inb(0x64) & 0x02) != 0 && timeout-- > 0);
    
    if (timeout > 0) {
        // 发送重启命令到8042
        outb(0x64, 0xFE);
        
        // 等待重启生效
        for (volatile int i = 0; i < 1000000; i++);
    }
    
    // 方法2: 通过Triple Fault（如果8042失败）
    print_string("8042 method failed, trying triple fault...\n");
    
#ifdef __x86_64__
    // 64位版本的triple fault
    __asm__ volatile (
        "cli\n"                    // 禁用中断
        "mov $0, %rax\n"           // 将0加载到RAX
        "lidt (%rax)\n"            // 加载无效的IDT
        "int $0\n"                 // 触发中断0（会导致triple fault）
    );
#else
    // 32位版本的triple fault
    __asm__ volatile (
        "cli\n"                    // 禁用中断
        "mov $0, %eax\n"           // 将0加载到EAX
        "lidt (%eax)\n"            // 加载无效的IDT
        "int $0\n"                 // 触发中断0（会导致triple fault）
    );
#endif
    
    // 方法3: 通过CPU重置（最后的尝试）
    print_string("Triple fault failed, trying CPU reset...\n");
    
#ifdef __x86_64__
    // 64位版本的CPU重置
    __asm__ volatile (
        "mov $0xFFFF, %rax\n"      // 加载段地址
        "mov $0x0000, %rbx\n"      // 加载偏移地址
        "push %rax\n"              // 压入段地址
        "push %rbx\n"              // 压入偏移地址
        "retf\n"                   // 远返回（跳转到重置向量）
    );
#else
    // 32位版本的CPU重置
    __asm__ volatile (
        "mov $0xFFFF, %eax\n"      // 加载段地址
        "mov $0x0000, %ebx\n"      // 加载偏移地址
        "push %eax\n"              // 压入段地址
        "push %ebx\n"              // 压入偏移地址
        "retf\n"                   // 远返回（跳转到重置向量）
    );
#endif
    
    // 如果所有方法都失败，进入无限循环
    print_string("All reboot methods failed. System halted.\n");
    while (1) {
        __asm__ volatile ("hlt");
    }
}
