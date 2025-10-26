// Boruix OS 进程切换实现
// 在中断中进行进程切换

#include "kernel/types.h"
#include "kernel/process.h"
#include "drivers/display.h"

// 进程上下文结构（与Rust中的ProcessContext一致）
typedef struct {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t int_no;
    uint64_t err_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} interrupt_context_t;

// Rust FFI函数声明
extern int rust_save_process_context(const interrupt_context_t* context);
extern const interrupt_context_t* rust_get_next_process_context(void);
extern bool rust_scheduler_tick(void);

// 系统时钟滴答计数
extern volatile uint32_t system_ticks;

// 是否启用进程切换
static bool process_switching_enabled = false;

// 调试计数器
static uint32_t switch_count = 0;

// 启用进程切换
void enable_process_switching(void) {
    process_switching_enabled = true;
    print_string("[SWITCH] Process switching enabled\n");
    
    extern void serial_puts(const char*);
    serial_puts("[SWITCH] Process switching flag set to true\n");
}

// 禁用进程切换
void disable_process_switching(void) {
    process_switching_enabled = false;
    print_string("[SWITCH] Process switching disabled\n");
}

// 定时器中断处理（支持进程切换）
// 参数：当前进程的上下文（栈上的寄存器）
// 返回值：0 = 不切换，非0 = 新进程上下文指针
void* timer_irq_handler_with_switch(interrupt_context_t* current_context) {
    // 增加系统时钟
    system_ticks++;
    
    // 如果进程切换未启用，不切换
    if (!process_switching_enabled) {
        return NULL;
    }
    
    // 调用调度器的tick函数，检查是否需要重新调度
    bool need_reschedule = rust_scheduler_tick();
    
    // 如果不需要重新调度，返回NULL（不切换）
    if (!need_reschedule) {
        return NULL;
    }
    
    // 保存当前进程的上下文到PCB
    if (rust_save_process_context(current_context) != 0) {
        // 保存失败，可能是第一次调度（没有当前进程）
        // 继续执行，尝试获取下一个进程
    }
    
    // 获取下一个进程的上下文
    const interrupt_context_t* next_context = rust_get_next_process_context();
    
    // 如果没有下一个进程，不切换
    if (next_context == NULL) {
        return NULL;
    }
    
    // 调试输出（每100次切换输出一次）
    // if (++switch_count % 100 == 0) {
    //     extern void serial_puts(const char*);
    //     extern void serial_put_dec(uint32_t);
    //     serial_puts("[SWITCH] Context switches: ");
    //     serial_put_dec(switch_count);
    //     serial_puts("\n");
    // }
    (void)switch_count;
    
    // 返回新进程的上下文指针
    // 汇编代码会切换到这个上下文
    return (void*)next_context;
}

