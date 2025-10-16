// Boruix OS 主内核文件
// 内核入口点和主要逻辑

#include "kernel/kernel.h"
#include "drivers/display.h"
#include "drivers/cmos.h"

// 内核主函数 - 现在接受multiboot参数
void kernel_main(unsigned int magic, unsigned int multiboot_info) {
    // 清屏
    clear_screen();
    
    // 显示欢迎信息
    print_string("Boruix Kernel\n");
    
    // 显示multiboot信息
    print_string("Multiboot Information:\n");
    print_string("- Received Magic: 0x");
    // 显示接收到的magic值
    char hex[] = "0123456789ABCDEF";
    unsigned int val = magic;
    for (int i = 7; i >= 0; i--) {
        print_char(hex[(val >> (i * 4)) & 0xF]);
    }
    print_string("\n");
    
    if (magic == 0x2BADB002) {
        print_string("- Magic: 0x2BADB002 (Valid)\n");
        print_string("- Multiboot Info: 0x");
        val = multiboot_info;
        for (int i = 7; i >= 0; i--) {
            print_char(hex[(val >> (i * 4)) & 0xF]);
        }
        print_string("\n");
    } else {
        print_string("- Magic: Invalid multiboot magic\n");
        print_string("- Expected: 0x2BADB002\n");
    }
    print_string("\n");
    
    // 简单的交互循环
    print_string("Kernel is running... System ready!\n");
    
    // 显示一个简单的动画
    for (int i = 0; i < 10; i++) {
        print_string(".");
        delay(1000000);
    }
    
    print_string("\n\nKernel initialization complete!\n");
    print_string("Boruix is now running successfully!\n");
    print_string("Current BIOS time:\n");
    
    // 显示初始时间
    print_current_time();
    print_string("\n");
    
    // 无限循环保持系统运行 - 每秒显示时间
    while (1) {
        // 等待约1秒（根据CPU频率调整）
        delay(10000000);
        
        // 打印当前时间并换行
        print_current_time();
        print_string("\n");
    }
}
