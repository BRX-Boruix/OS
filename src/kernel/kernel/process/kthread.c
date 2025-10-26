// Boruix OS 内核线程实现
// 提供创建和管理内核线程的功能

#include "kernel/process.h"
#include "kernel/types.h"
#include "drivers/display.h"

// 前向声明
void kthread_yield(void);

// 内核线程包装器结构
typedef struct {
    void (*entry)(void* arg);
    void* arg;
} kthread_wrapper_t;

// 内核线程入口包装函数
static void kthread_entry_wrapper(void) {
    // 从栈上获取包装器数据
    // 注意：这个实现是简化的，实际应该从进程的用户数据中获取
    print_string("[KTHREAD] Kernel thread started\n");
    
    // 无限循环等待实现
    while (1) {
        __asm__ volatile("hlt");
    }
}

// 创建内核线程
pid_t kthread_create(void (*entry)(void* arg), void* arg, const char* name, process_priority_t priority) {
    if (entry == NULL || name == NULL) {
        return INVALID_PID;
    }
    
    print_string("[KTHREAD] Creating kernel thread: ");
    print_string(name);
    print_string("\n");
    
    // 创建进程（使用包装器入口）
    pid_t pid = process_create(name, (void (*)(void))entry, priority);
    
    if (pid == INVALID_PID) {
        print_string("[KTHREAD] Failed to create kernel thread\n");
        return INVALID_PID;
    }
    
    print_string("[KTHREAD] Kernel thread created with PID: ");
    print_dec(pid);
    print_string("\n");
    
    return pid;
}

// 退出当前内核线程
void kthread_exit(int exit_code) {
    pid_t current_pid = process_get_current_pid();
    
    print_string("[KTHREAD] Thread ");
    print_dec(current_pid);
    print_string(" exiting with code ");
    print_dec(exit_code);
    print_string("\n");
    
    // 销毁当前进程
    process_destroy(current_pid);
    
    // 让出CPU
    scheduler_yield();
    
    // 不应该到达这里
    while (1) {
        __asm__ volatile("hlt");
    }
}

// 内核线程睡眠（简化实现）
void kthread_sleep(uint32_t milliseconds) {
    // 简化实现：通过多次让出CPU来模拟睡眠
    // 每次让出约10ms（一个时间片），所以需要让出 milliseconds/10 次
    uint32_t yields = milliseconds / 10;
    if (yields == 0) yields = 1;
    
    for (uint32_t i = 0; i < yields; i++) {
        kthread_yield();
    }
}

// 让出CPU
void kthread_yield(void) {
    // 通过软件中断（INT 0x80）立即触发进程切换
    // 这比等待定时器中断更快速和高效
    __asm__ volatile("int $0x80");
}

