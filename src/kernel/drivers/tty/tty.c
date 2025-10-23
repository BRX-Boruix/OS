// Boruix OS TTY系统主实现
// 整合TTY设备、会话和内核日志系统

#include "kernel/tty.h"
#include "drivers/display.h"

// TTY专用内存管理函数
extern void* tty_kmalloc(size_t size);
extern void tty_kfree(void* ptr);
extern void tty_memory_init(void);

// 初始化TTY系统
void tty_init(void) {
    print_string("[TTY] Starting TTY initialization...\n");
    
    // 初始化TTY专用内存管理
    tty_memory_init();
    print_string("[TTY] TTY memory management initialized\n");
    
    // 初始化内核日志系统
    klog_init();
    print_string("[TTY] Kernel log system initialized\n");
    
    // 初始化TTY设备系统
    tty_init_devices();
    print_string("[TTY] TTY devices initialized\n");
    
    // 检查默认设备
    tty_device_t *default_dev = tty_get_default_device();
    if (default_dev) {
        print_string("[TTY] Default device found: ");
        print_string(default_dev->name);
        print_string("\n");
    } else {
        print_string("[TTY] No default device found\n");
    }
    
    // 初始化内核TTY会话
    tty_init_kernel_session();
    
    // 输出初始化信息（使用基本输出确保可见）
    if (kernel_tty_session) {
        print_string("[INFO] TTY system initialized\n");
        print_string("[INFO] Default device: ");
        print_string(kernel_tty_session->device->name);
        print_string("\n");
    } else {
        print_string("[ERROR] TTY system initialization failed\n");
    }
}

// 获取TTY系统状态
bool tty_is_initialized(void) {
    return kernel_tty_session != NULL;
}

// 切换TTY设备
int tty_switch_device(const char *device_name) {
    if (!device_name || !kernel_tty_session) return -1;
    
    tty_device_t *device = tty_get_device(device_name);
    if (!device) return -1;
    
    return tty_set_session_device(kernel_tty_session, device);
}

// 获取当前设备名称
const char *tty_get_current_device_name(void) {
    if (!kernel_tty_session || !kernel_tty_session->device) return NULL;
    return kernel_tty_session->device->name;
}

// 获取设备列表（用于调试）
void tty_list_devices(void) {
    if (!kernel_tty_session) {
        kprint("TTY system not initialized\n");
        return;
    }
    
    kprint("Available TTY devices:\n");
    
    tty_device_t *current = tty_get_device("graphics");
    if (current) {
        kprint("  - graphics (default)\n");
    }
    
    current = tty_get_device("serial");
    if (current) {
        kprint("  - serial\n");
    }
    
    current = tty_get_device("vga");
    if (current) {
        kprint("  - vga\n");
    }
}

// 测试TTY功能
void tty_test(void) {
    if (!kernel_tty_session) {
        kprint("TTY system not initialized\n");
        return;
    }
    
    kprint("=== TTY System Test ===\n");
    
    // 测试基本输出
    kprint("Testing basic output...\n");
    
    // 测试颜色输出
    kprint("Testing color output:\n");
    kprint_color(TTY_COLOR_RED, TTY_COLOR_BLACK, "  Red text\n");
    kprint_color(TTY_COLOR_GREEN, TTY_COLOR_BLACK, "  Green text\n");
    kprint_color(TTY_COLOR_BLUE, TTY_COLOR_BLACK, "  Blue text\n");
    kprint_color(TTY_COLOR_YELLOW, TTY_COLOR_BLACK, "  Yellow text\n");
    kprint_color(TTY_COLOR_CYAN, TTY_COLOR_BLACK, "  Cyan text\n");
    kprint_color(TTY_COLOR_MAGENTA, TTY_COLOR_BLACK, "  Magenta text\n");
    
    // 测试日志级别
    kprint("Testing log levels:\n");
    kdebug("This is a debug message");
    kinfo("This is an info message");
    kwarn("This is a warning message");
    kerror("This is an error message");
    
    // 测试格式化输出
    kprint("Testing formatted output:\n");
    kprintf("  Decimal: %d\n", 12345);
    kprintf("  Hexadecimal: 0x%x\n", 0xABCDEF);
    kprintf("  String: %s\n", "Hello, TTY!");
    
    kprint("=== TTY Test Complete ===\n");
}

// 清理TTY系统
void tty_cleanup(void) {
    if (kernel_tty_session) {
        tty_destroy_session(kernel_tty_session);
        kernel_tty_session = NULL;
    }
    
    // 清理设备列表
    tty_device_t *current = tty_get_device("graphics");
    while (current) {
        tty_device_t *next = current->next;
        tty_unregister_device(current);
        if (current->private_data) {
            tty_kfree(current->private_data);
        }
        tty_kfree(current);
        current = next;
    }
}
