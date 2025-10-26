// Boruix OS 进程管理C语言包装器
// 为Rust实现的进程管理提供C接口

#include "kernel/process.h"
#include "kernel/types.h"
#include "drivers/display.h"

// 初始化进程管理系统
int process_init(void) {
    print_string("[PROCESS] Initializing process management system...\n");
    
    extern void serial_puts(const char*);
    serial_puts("[PROCESS] Calling rust_process_init()...\n");
    
    int result = rust_process_init();
    
    serial_puts("[PROCESS] rust_process_init() returned\n");
    
    if (result == 0) {
        print_string("[PROCESS] Process management initialized successfully\n");
    } else {
        print_string("[PROCESS] Failed to initialize process management\n");
    }
    
    return result;
}

// 创建新进程
pid_t process_create(const char* name, void (*entry_point)(void), process_priority_t priority) {
    if (name == NULL || entry_point == NULL) {
        return INVALID_PID;
    }
    
    // 计算名称长度
    size_t name_len = 0;
    while (name[name_len] != '\0' && name_len < 32) {
        name_len++;
    }
    
    return rust_create_process((const uint8_t*)name, name_len, 
                              (uintptr_t)entry_point, (uint8_t)priority);
}

// 销毁进程
int process_destroy(pid_t pid) {
    return rust_destroy_process(pid);
}

// 获取当前进程ID
pid_t process_get_current_pid(void) {
    return rust_get_current_pid();
}

// 获取进程信息
int process_get_info(pid_t pid, process_info_t* info) {
    if (info == NULL) {
        return -1;
    }
    return rust_get_process_info(pid, info);
}

// 设置进程优先级
int process_set_priority(pid_t pid, process_priority_t priority) {
    return rust_set_process_priority(pid, (uint8_t)priority);
}

// 获取进程数量
size_t process_get_count(void) {
    return rust_get_process_count();
}

// 执行调度
pid_t scheduler_schedule(void) {
    return rust_schedule();
}

// 时钟中断处理
bool scheduler_tick(void) {
    return rust_scheduler_tick();
}

// 进程让出CPU
pid_t scheduler_yield(void) {
    return rust_yield_cpu();
}

// 阻塞当前进程
void scheduler_block_current(void) {
    rust_block_current_process();
}

// 唤醒进程
void scheduler_wakeup(pid_t pid) {
    rust_wakeup_process(pid);
}

// 启用调度器
void scheduler_enable(void) {
    rust_enable_scheduler();
    print_string("[SCHEDULER] Scheduler enabled\n");
}

// 禁用调度器
void scheduler_disable(void) {
    rust_disable_scheduler();
    print_string("[SCHEDULER] Scheduler disabled\n");
}

// 设置调度策略
int scheduler_set_policy(sched_policy_t policy) {
    return rust_set_scheduling_policy((uint8_t)policy);
}

// 获取调度器统计信息
int scheduler_get_stats(scheduler_stats_t* stats) {
    if (stats == NULL) {
        return -1;
    }
    return rust_get_scheduler_stats(stats);
}

// 获取就绪队列大小
size_t scheduler_get_ready_queue_size(void) {
    return rust_get_ready_queue_size();
}

// 获取阻塞队列大小
size_t scheduler_get_blocked_queue_size(void) {
    return rust_get_blocked_queue_size();
}

// 执行上下文切换
int process_context_switch(pid_t from_pid, pid_t to_pid) {
    return rust_context_switch(from_pid, to_pid);
}

// 创建消息队列
ipc_id_t ipc_create_message_queue(pid_t owner) {
    return rust_create_message_queue(owner);
}

// 发送消息
int ipc_send_message(ipc_id_t queue_id, pid_t sender, pid_t receiver,
                    uint32_t msg_type, const void* data, size_t data_len) {
    if (data == NULL || data_len == 0) {
        return -1;
    }
    return rust_send_message(queue_id, sender, receiver, msg_type,
                           (const uint8_t*)data, data_len);
}

// 接收消息
int ipc_receive_message(ipc_id_t queue_id, void* buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size == 0) {
        return -1;
    }
    return rust_receive_message(queue_id, (uint8_t*)buffer, buffer_size);
}

