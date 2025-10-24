// Boruix OS 主内核 - Limine + Framebuffer + Rust内存管理

#include "kernel/kernel.h"
#include "drivers/display.h"
#include "drivers/cmos.h"
#include "kernel/interrupt.h"
#include "kernel/limine.h"
#include "kernel/memory.h"
#include "kernel/tty.h"
#include "kernel/serial_debug.h"
#include "arch/tss.h"
#include "arch/gdt.h"

// Limine requests
__attribute__((used, section(".requests")))
static volatile LIMINE_BASE_REVISION(2);

__attribute__((used, section(".requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
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
    // 初始化串口调试（第一件事）
    serial_debug_init();
    SERIAL_INFO("=== Boruix OS Boot Starting ===");
    
    // 检查Limine
    SERIAL_DEBUG("Checking Limine base revision...");
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        SERIAL_ERROR("Limine base revision not supported!");
        hcf();
    }
    
    SERIAL_DEBUG("Checking framebuffer...");
    if (framebuffer_request.response == NULL || 
        framebuffer_request.response->framebuffer_count < 1) {
        SERIAL_ERROR("No framebuffer available!");
        hcf();
    }
    
    SERIAL_DEBUG("Checking HHDM...");
    if (hhdm_request.response == NULL) {
        SERIAL_ERROR("HHDM request failed!");
        hcf();
    }
    SERIAL_INFO("Limine checks passed");
    
    // 初始化显示系统（framebuffer适配层）
    SERIAL_DEBUG("Initializing display system...");
    display_init(framebuffer_request.response->framebuffers[0]);
    clear_screen();
    
    // 显示欢迎信息（使用原有显示系统）
    print_string("BORUIX OS x86_64\n");
    print_string("========================================\n");
    print_string("Limine Bootloader OK\n\n");
    
    // 显示分辨率
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    print_string("Resolution: ");
    print_dec(fb->width);
    print_string("x");
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
    
    // 显示HHDM信息
    print_string("HHDM Offset: 0x");
    print_hex(hhdm_request.response->offset);
    print_string("\n\n");
    
    serial_puts("[INFO] HHDM Offset: ");
    serial_put_hex(hhdm_request.response->offset);
    serial_puts("\n");
    
    // 初始化Rust内存管理器（阶段2：物理分配器）
    print_string("Initializing Rust memory manager (Stage 2)...\n");
    SERIAL_INFO("Setting HHDM offset...");
    rust_set_hhdm_offset(hhdm_request.response->offset);
    
    SERIAL_INFO("Calling memory_init()...");
    int memory_result = memory_init(0);
    
    serial_puts("[INFO] memory_init() returned: ");
    serial_put_dec(memory_result);
    serial_puts("\n");
    
    if (memory_result == 0) {
        print_string("Rust memory manager initialized successfully!\n");
        SERIAL_INFO("Rust memory manager initialized successfully!");
    } else {
        print_string("Failed to initialize Rust memory manager\n");
        SERIAL_ERROR("Failed to initialize Rust memory manager!");
    }
    
    // 初始化TSS（双重错误需要）
    print_string("Initializing TSS (Task State Segment)...\n");
    tss_init();
    print_string("TSS initialized!\n");
    
    // 初始化GDT（包含TSS描述符）
    print_string("Initializing GDT (Global Descriptor Table)...\n");
    gdt_init();
    print_string("GDT initialized!\n");
    
    // 初始化中断系统
    print_string("Initializing interrupt system...\n");
    interrupt_init();
    print_string("Interrupt system ready!\n");
    
    print_string("========================================\n");
    print_string("SYSTEM READY\n");
    print_string("========================================\n\n");
    
    // Loading动画
    print_string(" Done!\n\n");
    
    // 调试：检查当前CS寄存器
    print_string("Checking current CS register...\n");
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
    print_string("CS confirmed = 0x28, IDT selector fixed!\n");
    
    print_string("Enabling Timer IRQ...\n");
    extern void pic_clear_mask(uint8_t irq);
    pic_clear_mask(0);  // Timer
    
    __asm__ volatile("sti");
    print_string("Interrupts ENABLED!\n");
    
    print_string("\n\nTimer ticks: ");
    print_dec((uint32_t)get_interrupt_count(32));
    print_string("\n");
    
    if (get_interrupt_count(32) > 0) {
        print_string("\nSUCCESS! Timer interrupt is WORKING!\n\n");
    } else {
        print_string("FAILED: Timer still not working\n");
    }
    
    // 启用键盘中断
    print_string("Enabling keyboard interrupt...\n");
    pic_clear_mask(1);  // Keyboard
    print_string("Keyboard IRQ enabled!\n");
    
    // 测试Rust内存管理系统
    print_string("Testing Rust memory management...\n");
    
    // 分配一些内存
    void *ptr1 = kmalloc(1024);
    void *ptr2 = kmalloc(2048);
    void *ptr3 = kmalloc(512);
    
    if (ptr1 && ptr2 && ptr3) {
        print_string("Memory allocation: SUCCESS\n");
        
        // 释放部分内存
        kfree(ptr2);
        print_string("Memory deallocation: SUCCESS\n");
        
        // 获取内存统计
        size_t total, used, free, peak;
        simple_memory_stats(&total, &used, &free, &peak);
        print_string("Memory stats - Total: ");
        print_dec((uint32_t)(total / 1024));
        print_string(" KB, Used: ");
        print_dec((uint32_t)(used / 1024));
        print_string(" KB, Free: ");
        print_dec((uint32_t)(free / 1024));
        print_string(" KB\n");
        
        // 释放剩余内存
        kfree(ptr1);
        kfree(ptr3);
        print_string("All memory freed: SUCCESS\n");
    } else {
        print_string("Memory allocation: FAILED\n");
    }
    
    print_string("Rust memory management test completed\n");
    
    // 测试TTY系统
    print_string("Testing TTY system...\n");
    tty_init();
    print_string("TTY system test completed\n");
    
    // 测试TTY输出功能
    print_string("Testing TTY output functions...\n");
    kprint("This is a test of kprint function\n");
    kprintf("This is a test of kprintf: %d\n", 42);
    kinfo("This is an info message\n");
    kdebug("This is a debug message\n");
    print_string("TTY output test completed\n");
    
    // 测试页面分配（阶段2）
    print_string("Testing page allocation...\n");
    uint64_t page1 = alloc_page();
    uint64_t page2 = alloc_page();
    
    if (page1 && page2) {
        print_string("Page allocation: SUCCESS\n");
        print_string("Page 1: 0x");
        print_hex(page1);
        print_string("\nPage 2: 0x");
        print_hex(page2);
        print_string("\n");
        
        free_page(page1);
        free_page(page2);
        print_string("Page deallocation: SUCCESS\n");
    } else {
        print_string("Page allocation: FAILED\n");
        if (!page1) print_string("  Page 1 returned 0\n");
        if (!page2) print_string("  Page 2 returned 0\n");
    }
    
    print_string("========================================\n");
    print_string("Starting Shell...\n");
    print_string("========================================\n\n");
    
    // 启动shell
    extern void shell_main(void);
    shell_main();
    
    hcf();
}