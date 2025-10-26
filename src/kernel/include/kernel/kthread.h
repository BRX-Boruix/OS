// Boruix OS 内核线程头文件
// 提供内核线程管理接口

#ifndef BORUIX_KTHREAD_H
#define BORUIX_KTHREAD_H

#include "kernel/types.h"
#include "kernel/process.h"

// 创建内核线程
// entry: 线程入口函数
// arg: 传递给线程的参数
// name: 线程名称
// priority: 线程优先级
// 返回: 线程PID，失败返回INVALID_PID
pid_t kthread_create(void (*entry)(void* arg), void* arg, const char* name, process_priority_t priority);

// 退出当前内核线程
// exit_code: 退出码
void kthread_exit(int exit_code);

// 内核线程睡眠
// milliseconds: 睡眠时间（毫秒）
void kthread_sleep(uint32_t milliseconds);

// 让出CPU
void kthread_yield(void);

#endif // BORUIX_KTHREAD_H

