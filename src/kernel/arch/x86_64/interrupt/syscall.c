// Boruix OS 系统调用处理
// 处理INT 0x80触发的系统调用

#include "kernel/interrupt.h"
#include "kernel/process.h"
#include "kernel/types.h"

// 前向声明Rust函数
extern int rust_save_process_context(const void* context);
extern const void* rust_get_next_process_context(void);
extern void rust_force_reschedule(void);

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

// 系统调用：进程让出CPU
// 参数：当前进程的上下文指针
// 返回值：0 = 不切换，非0 = 新进程上下文指针
void* syscall_yield_handler(interrupt_context_t* current_context) {
    // 保存当前进程的上下文到PCB
    if (rust_save_process_context(current_context) != 0) {
        // 保存失败，不切换
        return NULL;
    }
    
    // 强制重新调度
    extern void rust_force_reschedule(void);
    rust_force_reschedule();
    
    // 获取下一个进程的上下文
    const interrupt_context_t* next_context = rust_get_next_process_context();
    
    // 如果没有下一个进程，不切换
    if (next_context == NULL) {
        return NULL;
    }
    
    // 返回新进程的上下文指针
    // 汇编代码会切换到这个上下文
    return (void*)next_context;
}

