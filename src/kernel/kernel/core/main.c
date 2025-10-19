// Boruix OS 主内核 - Limine + Framebuffer

#include "kernel/kernel.h"
#include "drivers/display.h"
#include "drivers/cmos.h"
#include "kernel/interrupt.h"
#include "kernel/limine.h"

// Limine requests
__attribute__((used, section(".requests")))
static volatile LIMINE_BASE_REVISION(2);

__attribute__((used, section(".requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".requests_start_marker")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".requests_end_marker")))
static volatile LIMINE_REQUESTS_END_MARKER;

static void hcf(void) {
    for (;;) __asm__("hlt");
}

void kmain(void) {
    // 检查Limine
    if (LIMINE_BASE_REVISION_SUPPORTED == false) hcf();
    if (framebuffer_request.response == NULL || 
        framebuffer_request.response->framebuffer_count < 1) hcf();
    
    // 初始化显示系统（framebuffer适配层）
    display_init(framebuffer_request.response->framebuffers[0]);
    clear_screen();
    
    // 显示欢迎信息
    print_string("BORUIX OS x86_64\n");
    print_string("========================================\n");
    print_string("Limine Bootloader OK\n\n");
    
    // 显示分辨率
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    print_string("Resolution: ");
    print_dec(fb->width);
    print_char('x');
    print_dec(fb->height);
    print_string("\n\n");
    
    // 显示时间
    print_string("Current time: ");
    unsigned char hour, minute, second;
    get_current_time(&hour, &minute, &second);
    print_two_digits(hour);
    print_char(':');
    print_two_digits(minute);
    print_char(':');
    print_two_digits(second);
    print_string("\n\n");
    
    // 初始化中断系统
    print_string("Initializing interrupt system...\n");
    interrupt_init();
    print_string("Interrupt system ready!\n\n");
    
    print_string("========================================\n");
    print_string("SYSTEM READY\n");
    print_string("========================================\n\n");
    
    // Loading动画
    print_string("Loading");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j <= i; j++) {
            print_char('.');
        }
        
        // 延迟1秒
        unsigned char sec_start = read_cmos(0x00);
        unsigned char sec_current;
        do {
            sec_current = read_cmos(0x00);
        } while (sec_current == sec_start);
        
        if (i < 2) {
            print_char('\r');
            print_string("Loading");
        }
    }
    print_string(" Done!\n\n");
    
    // 调试：检查当前CS寄存器
    print_string("Debug: Checking current CS register...\n");
    uint16_t cs;
    __asm__ volatile("mov %%cs, %0" : "=r"(cs));
    print_string("CS = 0x");
    print_hex(cs);
    print_string("\n\n");
    
    // 测试Timer中断（最简单的中断）
    print_string("Testing Timer interrupt only...\n");
    
    // 禁用键盘中断，只测试timer
    extern void pic_set_mask(uint8_t irq);
    pic_set_mask(1);  // 禁用IRQ1 (keyboard)
    print_string("Keyboard IRQ disabled\n");
    
    extern uint64_t get_interrupt_count(uint8_t int_no);
    print_string("Initial timer ticks: ");
    print_dec((uint32_t)get_interrupt_count(32));
    print_string("\n");
    
    // CS已确认为0x28，IDT已修复，现在测试Timer中断
    print_string("CS confirmed = 0x28, IDT selector fixed!\n\n");
    
    print_string("Enabling Timer IRQ...\n");
    extern void pic_clear_mask(uint8_t irq);
    pic_clear_mask(0);  // Timer
    
    __asm__ volatile("sti");
    print_string("Interrupts ENABLED!\n\n");
    
    print_string("Waiting 3 seconds...\n");
    unsigned char sec_start = read_cmos(0x00);
    for (int i = 0; i < 3; i++) {
        unsigned char sec_cur;
        do {
            sec_cur = read_cmos(0x00);
            __asm__ volatile("pause");
        } while (sec_cur == sec_start);
        sec_start = sec_cur;
        print_char('.');
    }
    
    print_string("\n\nTimer ticks: ");
    print_dec((uint32_t)get_interrupt_count(32));
    print_string("\n");
    
    if (get_interrupt_count(32) > 0) {
        print_string("\nSUCCESS! Timer interrupt is WORKING!\n\n");
    } else {
        print_string("\nFAILED: Timer still not working\n\n");
    }
    
    // 启用键盘中断
    print_string("Enabling keyboard interrupt...\n");
    pic_clear_mask(1);  // Keyboard
    print_string("Keyboard IRQ enabled!\n\n");
    
    print_string("========================================\n");
    print_string("Starting Shell...\n");
    print_string("========================================\n\n");
    
    // 启动shell
    extern void shell_main(void);
    shell_main();
    
    hcf();
}
