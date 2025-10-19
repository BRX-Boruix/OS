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
    
    // 暂时不启用中断，先确认系统稳定
    print_string("System initialized successfully.\n");
    print_string("Interrupts remain DISABLED for stability.\n");
    print_string("System halted.\n\n");
    
    // 显示中断状态
    extern uint64_t get_interrupt_count(uint8_t int_no);
    print_string("Timer ticks: ");
    print_dec((uint32_t)get_interrupt_count(32));
    print_string("\n");
    print_string("Keyboard hits: ");
    print_dec((uint32_t)get_interrupt_count(33));
    print_string("\n\n");
    
    print_string("NOTE: Shell and interrupts temporarily disabled.\n");
    print_string("      Working on interrupt handler fixes.\n");
    
    hcf();
}
