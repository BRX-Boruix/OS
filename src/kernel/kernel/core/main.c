// Boruix OS 主内核 - Limine启动（VGA文本模式）

#include "kernel/kernel.h"
#include "drivers/display.h"
#include "drivers/cmos.h"
#include "kernel/shell.h"
#include "kernel/interrupt.h"
#include "kernel/limine.h"

// Limine base revision
__attribute__((used, section(".requests")))
static volatile LIMINE_BASE_REVISION(2);

// Start和End markers
__attribute__((used, section(".requests_start_marker")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".requests_end_marker")))
static volatile LIMINE_REQUESTS_END_MARKER;

// Halt and catch fire
static void hcf(void) {
    for (;;) {
        __asm__("hlt");
    }
}

// 内核入口点
void kmain(void) {
    // 检查Limine base revision
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }
    
    // 初始化显示
    clear_screen();
    
    // 显示欢迎信息
    print_string("BORUIX OS x86_64 - Limine Bootloader\n");
    print_string("========================================\n\n");
    
    // 显示时间
    print_string("Current time: ");
    print_current_time();
    print_string("\n\n");
    
    // 初始化中断
    interrupt_init();
    print_string("\n");
    
    print_string("Kernel loaded successfully!\n");
    print_string("Ready\n\n");
    
    // 等待3秒
    for (int i = 0; i < 3; i++) {
        print_string("Loading");
        for (int j = 0; j <= i; j++) {
            print_char('.');
        }
        print_string("\r");
        uint8_t sec_origin = read_cmos(0x00);
        uint8_t sec_cur;
        do {
            sec_cur = read_cmos(0x00);
        } while (sec_cur == sec_origin);
    }
    print_string("Done       \n");
    clear_screen();
    
    // 启动shell
    shell_main();
}
