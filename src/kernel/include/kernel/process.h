// Boruix OS 进程管理头文件
// 提供进程管理的C语言接口

#ifndef BORUIX_PROCESS_H
#define BORUIX_PROCESS_H

#include "kernel/types.h"

// 进程ID类型
typedef uint32_t pid_t;

// 无效进程ID
#define INVALID_PID 0

// 进程状态枚举
typedef enum {
    PROCESS_CREATED = 0,
    PROCESS_READY = 1,
    PROCESS_RUNNING = 2,
    PROCESS_BLOCKED = 3,
    PROCESS_ZOMBIE = 4,
    PROCESS_TERMINATED = 5,
} process_state_t;

// 进程优先级枚举
typedef enum {
    PRIORITY_REALTIME = 0,
    PRIORITY_HIGH = 1,
    PRIORITY_NORMAL = 2,
    PRIORITY_LOW = 3,
    PRIORITY_IDLE = 4,
} process_priority_t;

// 调度策略枚举
typedef enum {
    SCHED_ROUND_ROBIN = 0,
    SCHED_PRIORITY = 1,
    SCHED_MULTILEVEL_FEEDBACK = 2,
} sched_policy_t;

// 进程信息结构
typedef struct {
    pid_t pid;
    pid_t parent_pid;
    uint8_t state;
    uint8_t priority;
    char name[32];
    uint64_t cpu_time;
    uint64_t created_at;
} process_info_t;

// 调度器统计信息结构
typedef struct {
    uint64_t total_schedules;
    uint64_t context_switches;
    uint64_t preemptions;
    uint64_t idle_time;
    uint64_t priority_schedules[5];
} scheduler_stats_t;

// IPC相关类型
typedef uint32_t ipc_id_t;
#define INVALID_IPC_ID 0

// ========== 进程管理函数 ==========

// 初始化进程管理系统
int process_init(void);

// 创建新进程
pid_t process_create(const char* name, void (*entry_point)(void), process_priority_t priority);

// 销毁进程
int process_destroy(pid_t pid);

// 获取当前进程ID
pid_t process_get_current_pid(void);

// 获取进程信息
int process_get_info(pid_t pid, process_info_t* info);

// 设置进程优先级
int process_set_priority(pid_t pid, process_priority_t priority);

// 获取进程数量
size_t process_get_count(void);

// ========== 调度器函数 ==========

// 执行调度
pid_t scheduler_schedule(void);

// 时钟中断处理
bool scheduler_tick(void);

// 进程让出CPU
pid_t scheduler_yield(void);

// 阻塞当前进程
void scheduler_block_current(void);

// 唤醒进程
void scheduler_wakeup(pid_t pid);

// 启用调度器
void scheduler_enable(void);

// 禁用调度器
void scheduler_disable(void);

// 设置调度策略
int scheduler_set_policy(sched_policy_t policy);

// 获取调度器统计信息
int scheduler_get_stats(scheduler_stats_t* stats);

// 获取就绪队列大小
size_t scheduler_get_ready_queue_size(void);

// 获取阻塞队列大小
size_t scheduler_get_blocked_queue_size(void);

// ========== 上下文切换函数 ==========

// 执行上下文切换
int process_context_switch(pid_t from_pid, pid_t to_pid);

// ========== IPC函数 ==========

// 创建消息队列
ipc_id_t ipc_create_message_queue(pid_t owner);

// 发送消息
int ipc_send_message(ipc_id_t queue_id, pid_t sender, pid_t receiver, 
                     uint32_t msg_type, const void* data, size_t data_len);

// 接收消息
int ipc_receive_message(ipc_id_t queue_id, void* buffer, size_t buffer_size);

// ========== Rust FFI声明 ==========

// 这些函数由Rust实现
extern int rust_process_init(void);
extern pid_t rust_create_process(const uint8_t* name, size_t name_len, 
                                 uintptr_t entry_point, uint8_t priority);
extern int rust_destroy_process(pid_t pid);
extern pid_t rust_get_current_pid(void);
extern pid_t rust_schedule(void);
extern bool rust_scheduler_tick(void);
extern pid_t rust_yield_cpu(void);
extern void rust_block_current_process(void);
extern void rust_wakeup_process(pid_t pid);
extern int rust_get_process_info(pid_t pid, process_info_t* info);
extern int rust_get_scheduler_stats(scheduler_stats_t* stats);
extern int rust_set_process_priority(pid_t pid, uint8_t priority);
extern size_t rust_get_process_count(void);
extern size_t rust_get_ready_queue_size(void);
extern size_t rust_get_blocked_queue_size(void);
extern int rust_set_scheduling_policy(uint8_t policy);
extern void rust_enable_scheduler(void);
extern void rust_disable_scheduler(void);
extern int rust_context_switch(pid_t from_pid, pid_t to_pid);
extern ipc_id_t rust_create_message_queue(pid_t owner);
extern int rust_send_message(ipc_id_t queue_id, pid_t sender, pid_t receiver,
                            uint32_t msg_type, const uint8_t* data, size_t data_len);
extern int rust_receive_message(ipc_id_t queue_id, uint8_t* buffer, size_t buffer_size);

// 内存分配函数（由内存管理器提供）
extern void* rust_allocate_pages(size_t count);
extern void rust_free_pages(void* ptr, size_t count);

#endif // BORUIX_PROCESS_H
