// Boruix OS Shell - 系统工具函数实现
// 提供系统级别的功能

#include "system.h"
#include "drivers/display.h"

// 简单的端口输出函数
static void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a" (value), "Nd" (port));
}

// 简单的端口输出函数（16位）
static void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a" (value), "Nd" (port));
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

// ACPI关机实现 - 写入PM1a Control Block
void acpi_shutdown(void) {
    print_string("[SHUTDOWN] Attempting ACPI shutdown...\n");
    
    // 尝试多个常见的ACPI PM1a Control Block地址
    uint16_t pm1a_addresses[] = {
        0x404,   // QEMU/KVM标准地址
        0x4004,  // 某些虚拟机地址
        0xB004,  // 某些真实硬件
        0x0404,  // 备选地址
    };
    
    // ACPI Sleep Control Register位定义:
    // Bits [12:10] = Sleep Type (SLP_TYP) for S5 state = 0x5
    // Bit [13] = Sleep Enable (SLP_EN) = 1
    // S5 (Soft Off): SLP_TYP=5, SLP_EN=1 => 0x1A00 (bit 13=1, bit 12-10=101b)
    
    uint16_t shutdown_value = (0x5 << 10) | (1 << 13);
    
    // 等待一下让用户看到消息
    for (volatile int i = 0; i < 5000000; i++);
    
    // 尝试所有已知的地址
    for (int i = 0; i < 4; i++) {
        print_string("[SHUTDOWN] Trying PM1a at 0x");
        uint16_t addr = pm1a_addresses[i];
        // 简单打印地址的十六进制
        if (addr >= 0x1000) print_string("1");
        print_string("000\n");
        
        // 向PM1a Control Block写入关机命令
        __asm__ volatile ("outw %0, %1" : : "a" (shutdown_value), "Nd" (pm1a_addresses[i]));
        
        // 等待关机生效
        for (volatile int i = 0; i < 1000000; i++);
    }
    
    print_string("[SHUTDOWN] ACPI shutdown command sent.\n");
}

// 传统关机实现 - 通过多个端口组合
void legacy_shutdown(void) {
    print_string("[SHUTDOWN] Attempting legacy power control...\n");
    
    // 方法1: 通过QEMU exit device (ISA IO port 0x501)
    print_string("[SHUTDOWN] Trying QEMU exit device port 0x501...\n");
    outb(0x501, 0x00);  // QEMU会识别这个端口并退出
    
    for (volatile int i = 0; i < 100000; i++);
    
    // 方法2: 通过Bochs/QEMU magic exit (0x8900)
    print_string("[SHUTDOWN] Trying Bochs magic exit...\n");
    outw(0x8900, 0x2000);
    
    for (volatile int i = 0; i < 100000; i++);
    
    // 方法3: CF9端口关机
    print_string("[SHUTDOWN] CF9 port: attempting system power off...\n");
    
    // 设置CF9寄存器以关闭系统
    // Bit 2 = System Reset Enable
    // Bit 1 = CPU Reset (Full Reset)
    // Bit 0 = (System Reset)
    
    outb(0xCF9, 0x00);
    
    for (volatile int i = 0; i < 100000; i++);
    
    // 方法4: 通过8042键盘控制器的另一种方式
    print_string("[SHUTDOWN] Trying 8042 keyboard controller...\n");
    
    int timeout = 100000;
    while ((inb(0x64) & 0x02) != 0 && timeout-- > 0);
    
    if (timeout > 0) {
        // 尝试通过键盘控制器执行关机相关操作
        outb(0x64, 0xAA);  // 8042自测命令
        for (volatile int i = 0; i < 100000; i++);
    }
    
    print_string("[SHUTDOWN] Legacy method completed.\n");
}

// 通用关机 - 多层次兜底策略
void shutdown_system(void) {
    print_string("Shutting down system...\n");
    print_string("Goodbye!\n");
    
    // 禁用中断
    __asm__ volatile ("cli");
    
    // 策略1: 直接尝试QEMU poweroff (port 0x604)
    print_string("[SHUTDOWN] Attempting direct QEMU poweroff (port 0x604)...\n");
    for (volatile int i = 0; i < 1000000; i++);
    
    // QEMU ACPI shutdown - 写入ACPI Power Control Register
    outw(0x604, 0x0600);  // S5 sleep type
    for (volatile int i = 0; i < 1000000; i++);
    
    // 策略2: ACPI关机
    acpi_shutdown();
    
    // 如果ACPI失败，尝试传统方法
    print_string("[SHUTDOWN] ACPI failed, trying legacy methods...\n");
    legacy_shutdown();
    
    // 策略3: 禁用所有中断并执行无限HLT循环
    print_string("[SHUTDOWN] All power-off methods attempted.\n");
    print_string("[SHUTDOWN] System halted - CPU in low-power state.\n");    
    // 禁用所有中断
    __asm__ volatile ("cli");
    
    // 无限HLT循环 - CPU进入停止状态
    while (1) {
        __asm__ volatile ("hlt");
    }
}
