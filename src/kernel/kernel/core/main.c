// Boruix OS 主内核文件
// 内核入口点和主要逻辑

#include "kernel/kernel.h"
#include "drivers/display.h"
#include "drivers/cmos.h"
#include "kernel/memory.h"
#include "kernel/shell.h"

// 显示十六进制数值的辅助函数
static void print_hex(uint32_t value) {
    char hex[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        print_char(hex[(value >> (i * 4)) & 0xF]);
    }
}

#ifdef __x86_64__
static void print_hex64(uint64_t value) {
    char hex[] = "0123456789ABCDEF";
    for (int i = 15; i >= 0; i--) {
        print_char(hex[(value >> (i * 4)) & 0xF]);
    }
}
#endif

// 显示启动信息
static void show_boot_info(uint32_t magic, uintptr_t multiboot_info) {
    // 显示欢迎信息
#ifdef __x86_64__
    print_string("Boruix OS x86_64 - 64-bit Kernel\n");
#else
    print_string("Boruix OS i386 - 32-bit Kernel\n");
#endif
    print_string("========================================\n\n");
    
    // 显示multiboot信息
    print_string("Boot Information:\n");
    print_string("- Magic: 0x");
    print_hex(magic);
    
    if (magic == 0x2BADB002) {
        print_string(" (Valid)\n");
        print_string("- Multiboot Info: 0x");
#ifdef __x86_64__
        print_hex64(multiboot_info);
#else
        print_hex(multiboot_info);
#endif
        print_string("\n");
    } else {
        print_string(" (Invalid - Expected 0x2BADB002)\n");
    }
    print_string("\n");
}

// 内核主函数 - 接受multiboot参数，支持双架构
#ifdef __x86_64__
void kernel_main(uint32_t magic, uint64_t multiboot_info) {
#else
void kernel_main(uint32_t magic, uint32_t multiboot_info) {
#endif
    // 初始化显示
    clear_screen();
    
    // 显示启动信息
    show_boot_info(magic, multiboot_info);
    
    // 系统初始化
    print_string("System Initialization:\n");
    print_string("- Display driver: OK\n");
    print_string("- CMOS driver: OK\n");
    print_string("- Memory management: DISABLED (debug mode)\n");
    print_string("- Kernel shell: Initializing...\n");
    print_string("\n");
    
    // 显示当前时间
    print_string("Current time: ");
    print_current_time();
    print_string("\n\n");
    
    print_string("Boruix OS is ready!\n");
    print_string("Type 'help' for available commands.\n\n");
    
    // 启动shell
    shell_main();
}
