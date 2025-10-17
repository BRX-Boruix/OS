// Boruix OS 主内核文件
// 内核入口点和主要逻辑

#include "kernel/kernel.h"
#include "drivers/display.h"
#include "drivers/cmos.h"
#include "kernel/memory.h"
#include "kernel/shell.h"
#include "kernel/interrupt.h"

// print_hex和print_dec现在在display.c中实现

// 显示启动信息
static void show_boot_info(uint32_t magic, uintptr_t multiboot_info) {
    // 显示欢迎信息
#ifdef __x86_64__
    print_string("BORUIX KERNEL x86_64\n");
#else
    print_string("BORUIX KERNEL i386\n");
#endif
    print_string("========================================\n\n");
    
    // 显示multiboot信息
    print_string("Boot Information:\n");
    print_string("- Magic: 0x");
    print_hex(magic);
    
    if (magic == 0x2BADB002) {
        print_string(" (Valid)\n");
        print_string("- Multiboot Info: ");
        print_hex((uint32_t)multiboot_info);
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
    
    // 显示当前时间
    print_string("Current time: ");
    print_current_time();
    print_string("\n\n");
    
    // 初始化中断系统
    interrupt_init();
    print_string("\n");
    
    print_string("Ready\n");

    // 等待3秒，每秒输出动画（使用实际CMOS时间，禁止写死）
    for (int i = 0; i < 3; i++) {
        print_string("Loading");
        for (int j = 0; j <= i; j++) {
            print_char('.');
        }
        print_string("\r");  // 回到行首，覆盖动画
        uint8_t sec_origin = read_cmos(0x00);
        uint8_t sec_cur;
        do {
            sec_cur = read_cmos(0x00);
        } while (sec_cur == sec_origin);
    }
    print_string("Done\n");
    clear_screen();

    // 启动shell
    shell_main();
}
