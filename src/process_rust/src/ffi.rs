//! FFI接口，为C代码提供进程管理功能

use crate::*;
use core::slice;
use core::ptr;

/// C兼容的进程信息结构
#[repr(C)]
pub struct CProcessInfo {
    pub pid: ProcessId,
    pub parent_pid: ProcessId,
    pub state: u8, // ProcessState as u8
    pub priority: u8, // Priority as u8
    pub name: [u8; 32],
    pub cpu_time: u64,
    pub created_at: u64,
}

/// C兼容的调度器统计信息
#[repr(C)]
pub struct CSchedulerStats {
    pub total_schedules: u64,
    pub context_switches: u64,
    pub preemptions: u64,
    pub idle_time: u64,
    pub priority_schedules: [u64; 5],
}

/// 初始化进程管理系统
#[no_mangle]
pub extern "C" fn rust_process_init() -> i32 {
    let mut manager = ProcessManager::new();
    
    match manager.init() {
        Ok(()) => {
            ProcessManager::set_instance(manager);
            0
        }
        Err(_) => -1,
    }
}

/// 创建新进程
#[no_mangle]
pub extern "C" fn rust_create_process(
    name: *const u8,
    name_len: usize,
    entry_point: usize,
    priority: u8,
) -> ProcessId {
    if name.is_null() || name_len == 0 {
        return INVALID_PID;
    }
    
    let name_slice = unsafe { slice::from_raw_parts(name, name_len) };
    let name_str = match core::str::from_utf8(name_slice) {
        Ok(s) => s,
        Err(_) => return INVALID_PID,
    };
    
    // 分配PID
    let manager = ProcessManager::instance();
    let pid = match manager.allocate_pid() {
        Ok(pid) => pid,
        Err(_) => return INVALID_PID,
    };
    
    // 创建PCB
    let mut pcb = match pcb::ProcessControlBlock::new(pid, name_str) {
        Ok(pcb) => pcb,
        Err(_) => return INVALID_PID,
    };
    
    // 设置优先级
    if let Some(prio) = scheduler::Priority::from_u8(priority) {
        pcb.set_priority(prio);
    }
    
    // 设置入口点
    pcb.set_entry_point(entry_point);
    
    // 分配内核栈
    if pcb.allocate_kernel_stack().is_err() {
        return INVALID_PID;
    }
    
    // 初始化上下文
    if let Some(stack_ptr) = pcb.kernel_stack_ptr() {
        pcb.context_mut().init_kernel_context(entry_point, stack_ptr);
    }
    
    // 设置为就绪状态
    pcb.set_state(state::ProcessState::Ready);
    
    // 扩展进程表到所需大小
    while manager.process_table.len() <= pid as usize {
        if manager.process_table.push(None).is_err() {
            return INVALID_PID;
        }
    }
    
    // 添加到进程表
    manager.process_table[pid as usize] = Some(pcb);
    
    // 添加到就绪队列
    manager.ready_queue.enqueue(pid).ok();
    
    pid
}

/// 销毁进程
#[no_mangle]
pub extern "C" fn rust_destroy_process(pid: ProcessId) -> i32 {
    let manager = ProcessManager::instance();
    
    if let Some(pcb) = manager.get_process_mut(pid) {
        // 设置为终止状态
        pcb.set_state(state::ProcessState::Terminated);
        
        // 从队列中移除
        manager.ready_queue.remove(pid);
        manager.blocked_queue.remove(pid);
        
        // 如果杀死的是当前进程，清除调度器的当前进程
        if let Some(current_pid) = manager.scheduler.current_process() {
            if current_pid == pid {
                manager.scheduler.set_current_process(None);
            }
        }
        
        // 清理PCB
        if (pid as usize) < manager.process_table.len() {
            manager.process_table[pid as usize] = None;
        }
        
        // 如果杀死的是关键系统进程，触发panic
        if pid <= 2 {
            extern "C" {
                fn serial_puts(s: *const u8);
            }
            let msg: &[u8] = match pid {
                0 => b"[CRITICAL] Kernel process (PID 0) terminated! System will crash.\n\0",
                1 => b"[CRITICAL] Idle process (PID 1) terminated! System will crash..\n\0",
                2 => b"[CRITICAL] Init process (PID 2) terminated! System will crash..\n\0",
                _ => b"[CRITICAL] System process terminated! System will crash........\n\0",
            };
            unsafe {
                serial_puts(msg.as_ptr());
            }
            // 触发panic
            panic!("Critical system process terminated!");
        }
        
        0
    } else {
        -1
    }
}

/// 获取当前进程ID
#[no_mangle]
pub extern "C" fn rust_get_current_pid() -> ProcessId {
    let manager = ProcessManager::instance();
    manager.current_pid().unwrap_or(INVALID_PID)
}

/// 进程调度
#[no_mangle]
pub extern "C" fn rust_schedule() -> ProcessId {
    let manager = ProcessManager::instance();
    
    // 从就绪队列中选择下一个进程
    if let Some(next_pid) = manager.ready_queue.dequeue() {
        // 如果有当前进程且不是同一个，将其放回就绪队列
        if let Some(current_pid) = manager.scheduler.current_process() {
            if current_pid != next_pid {
                // 检查当前进程是否还在运行状态
                if let Some(pcb) = manager.get_process(current_pid) {
                    if pcb.state() == state::ProcessState::Running {
                        // 将当前进程设置为就绪状态并放回队列
                        if let Some(pcb_mut) = manager.get_process_mut(current_pid) {
                            pcb_mut.set_state(state::ProcessState::Ready);
                        }
                        let _ = manager.ready_queue.enqueue(current_pid);
                    }
                }
            }
        }
        
        // 设置新进程为运行状态
        if let Some(pcb) = manager.get_process_mut(next_pid) {
            pcb.set_state(state::ProcessState::Running);
        }
        
        // 更新调度器状态
        manager.scheduler.set_current_process(Some(next_pid));
        manager.scheduler.record_schedule();
        
        next_pid
    } else {
        // 没有就绪进程，返回idle进程（PID 1）
        manager.scheduler.set_current_process(Some(1));
        1
    }
}

/// 时钟中断处理
#[no_mangle]
pub extern "C" fn rust_scheduler_tick() -> bool {
    let manager = ProcessManager::instance();
    manager.scheduler.tick()
}

/// 进程让出CPU
#[no_mangle]
pub extern "C" fn rust_yield_cpu() -> ProcessId {
    let manager = ProcessManager::instance();
    manager.scheduler.yield_cpu().unwrap_or(INVALID_PID)
}

/// 强制重新调度（将时间片设为0）
#[no_mangle]
pub extern "C" fn rust_force_reschedule() {
    let manager = ProcessManager::instance();
    manager.scheduler.force_reschedule();
}

/// 阻塞当前进程
#[no_mangle]
pub extern "C" fn rust_block_current_process() {
    let manager = ProcessManager::instance();
    manager.scheduler.block_current();
}

/// 唤醒进程
#[no_mangle]
pub extern "C" fn rust_wakeup_process(pid: ProcessId) {
    let manager = ProcessManager::instance();
    manager.scheduler.wakeup_process(pid);
}

/// 获取进程信息
#[no_mangle]
pub extern "C" fn rust_get_process_info(pid: ProcessId, info: *mut CProcessInfo) -> i32 {
    if info.is_null() {
        return -1;
    }
    
    let manager = ProcessManager::instance();
    
    if let Some(pcb) = manager.get_process(pid) {
        let c_info = CProcessInfo {
            pid: pcb.pid(),
            parent_pid: pcb.parent_pid(),
            state: pcb.state() as u8,
            priority: pcb.priority() as u8,
            name: {
                let mut name = [0u8; 32];
                let pcb_name = pcb.name().as_bytes();
                let len = core::cmp::min(pcb_name.len(), 31);
                name[..len].copy_from_slice(&pcb_name[..len]);
                name
            },
            cpu_time: pcb.cpu_time(),
            created_at: pcb.created_at,
        };
        
        unsafe {
            ptr::write(info, c_info);
        }
        
        0
    } else {
        -1
    }
}

/// 获取调度器统计信息
#[no_mangle]
pub extern "C" fn rust_get_scheduler_stats(stats: *mut CSchedulerStats) -> i32 {
    if stats.is_null() {
        return -1;
    }
    
    let manager = ProcessManager::instance();
    let scheduler_stats = manager.scheduler.stats();
    
    let c_stats = CSchedulerStats {
        total_schedules: scheduler_stats.total_schedules,
        context_switches: scheduler_stats.context_switches,
        preemptions: scheduler_stats.preemptions,
        idle_time: scheduler_stats.idle_time,
        priority_schedules: scheduler_stats.priority_schedules,
    };
    
    unsafe {
        ptr::write(stats, c_stats);
    }
    
    0
}

/// 设置进程优先级
#[no_mangle]
pub extern "C" fn rust_set_process_priority(pid: ProcessId, priority: u8) -> i32 {
    let manager = ProcessManager::instance();
    
    if let Some(pcb) = manager.get_process_mut(pid) {
        if let Some(prio) = scheduler::Priority::from_u8(priority) {
            pcb.set_priority(prio);
            0
        } else {
            -1
        }
    } else {
        -1
    }
}

/// 获取进程数量
#[no_mangle]
pub extern "C" fn rust_get_process_count() -> usize {
    let manager = ProcessManager::instance();
    manager.process_table.iter().filter(|pcb_opt| pcb_opt.is_some()).count()
}

/// 获取就绪队列大小
#[no_mangle]
pub extern "C" fn rust_get_ready_queue_size() -> usize {
    let manager = ProcessManager::instance();
    manager.ready_queue.len()
}

/// 获取阻塞队列大小
#[no_mangle]
pub extern "C" fn rust_get_blocked_queue_size() -> usize {
    let manager = ProcessManager::instance();
    manager.blocked_queue.len()
}

/// 设置调度策略
#[no_mangle]
pub extern "C" fn rust_set_scheduling_policy(policy: u8) -> i32 {
    let manager = ProcessManager::instance();
    
    let sched_policy = match policy {
        0 => scheduler::SchedulingPolicy::RoundRobin,
        1 => scheduler::SchedulingPolicy::Priority,
        2 => scheduler::SchedulingPolicy::MultilevelFeedback,
        _ => return -1,
    };
    
    manager.scheduler.set_policy(sched_policy);
    0
}

/// 启用调度器
#[no_mangle]
pub extern "C" fn rust_enable_scheduler() {
    let manager = ProcessManager::instance();
    manager.scheduler.enable();
}

/// 禁用调度器
#[no_mangle]
pub extern "C" fn rust_disable_scheduler() {
    let manager = ProcessManager::instance();
    manager.scheduler.disable();
}

/// 执行上下文切换
#[no_mangle]
pub extern "C" fn rust_context_switch(from_pid: ProcessId, to_pid: ProcessId) -> i32 {
    let manager = ProcessManager::instance();
    
    // 获取源进程上下文
    let from_context = match manager.get_process_mut(from_pid) {
        Some(from_pcb) => from_pcb.context_mut() as *mut _,
        None => return -1,
    };
    
    // 获取目标进程上下文
    let to_context = match manager.get_process(to_pid) {
        Some(to_pcb) => to_pcb.context() as *const _,
        None => return -1,
    };
    
    // 执行上下文切换
    unsafe {
        context::switch_context(from_context, to_context);
    }
    
    0
}

/// 创建消息队列
#[no_mangle]
pub extern "C" fn rust_create_message_queue(_owner: ProcessId) -> IpcId {
    // TODO: 实现IPC管理器的全局实例
    INVALID_IPC_ID
}

/// 发送消息
#[no_mangle]
pub extern "C" fn rust_send_message(
    _queue_id: IpcId,
    sender: ProcessId,
    receiver: ProcessId,
    msg_type: u32,
    data: *const u8,
    data_len: usize,
) -> i32 {
    if data.is_null() || data_len == 0 {
        return -1;
    }
    
    let data_slice = unsafe { slice::from_raw_parts(data, data_len) };
    
    match ipc::Message::new(sender, receiver, msg_type, data_slice) {
        Ok(_message) => {
            // TODO: 实际发送消息
            0
        }
        Err(_) => -1,
    }
}

/// 接收消息
#[no_mangle]
pub extern "C" fn rust_receive_message(
    _queue_id: IpcId,
    buffer: *mut u8,
    buffer_size: usize,
) -> i32 {
    if buffer.is_null() || buffer_size == 0 {
        return -1;
    }
    
    // TODO: 实现消息接收
    -1
}

/// 保存当前进程的上下文（从中断栈帧）
/// 参数：current_context - 指向中断栈帧的指针（与ProcessContext结构一致）
#[no_mangle]
pub extern "C" fn rust_save_process_context(current_context: *const context::ProcessContext) -> i32 {
    if current_context.is_null() {
        return -1;
    }
    
    let manager = ProcessManager::instance();
    
    // 获取当前进程ID
    let current_pid = match manager.scheduler.current_process() {
        Some(pid) => pid,
        None => return -1,
    };
    
    // 获取当前进程的PCB
    let pcb = match manager.get_process_mut(current_pid) {
        Some(pcb) => pcb,
        None => return -1,
    };
    
    // 复制上下文
    unsafe {
        *pcb.context_mut() = *current_context;
    }
    
    0
}

/// 选择优先级最高的就绪进程
fn select_highest_priority_process(manager: &mut ProcessManager) -> ProcessId {
    let mut best_pid: Option<ProcessId> = None;
    let mut best_priority: Option<scheduler::Priority> = None;
    let mut candidates: Vec<(ProcessId, scheduler::Priority), 256> = Vec::new();
    
    // 收集所有就绪进程
    while let Some(pid) = manager.ready_queue.dequeue() {
        if let Some(pcb) = manager.get_process(pid) {
            if pcb.state() == state::ProcessState::Ready {
                candidates.push((pid, pcb.priority())).ok();
            }
        }
    }
    
    // 如果没有就绪进程，返回idle进程
    if candidates.is_empty() {
        return 1;
    }
    
    // 选择优先级最高的进程（数字越小优先级越高）
    for (pid, priority) in candidates.iter() {
        match best_priority {
            None => {
                best_pid = Some(*pid);
                best_priority = Some(*priority);
            }
            Some(current_best) => {
                if *priority < current_best {
                    best_pid = Some(*pid);
                    best_priority = Some(*priority);
                }
            }
        }
    }
    
    // 将未选中的进程放回就绪队列
    for (pid, _) in candidates.iter() {
        if Some(*pid) != best_pid {
            manager.ready_queue.enqueue(*pid).ok();
        }
    }
    
    best_pid.unwrap_or(1)
}

/// 获取下一个进程的上下文指针（带优先级调度）
/// 返回值：进程上下文指针，如果没有下一个进程则返回null
#[no_mangle]
pub extern "C" fn rust_get_next_process_context() -> *const context::ProcessContext {
    let manager = ProcessManager::instance();
    
    // 使用优先级调度：选择优先级最高的就绪进程
    let next_pid = select_highest_priority_process(manager);
    
    // 获取当前进程PID（用于记录上下文切换）
    let current_pid = manager.scheduler.current_process().unwrap_or(0);
    
    // 如果有当前进程且不是同一个，将其放回就绪队列
    if current_pid != 0 && current_pid != next_pid {
        // 检查当前进程是否还在运行状态
        if let Some(pcb) = manager.get_process(current_pid) {
            if pcb.state() == state::ProcessState::Running {
                // 将当前进程设置为就绪状态并放回队列
                if let Some(pcb_mut) = manager.get_process_mut(current_pid) {
                    pcb_mut.set_state(state::ProcessState::Ready);
                }
                let _ = manager.ready_queue.enqueue(current_pid);
            }
        }
    }
    
    // 设置新进程为运行状态
    if let Some(pcb) = manager.get_process_mut(next_pid) {
        pcb.set_state(state::ProcessState::Running);
    }
    
    // 记录上下文切换（这会更新统计信息）
    manager.scheduler.record_context_switch(current_pid, next_pid);
    manager.scheduler.record_schedule();
    
    // 获取新进程的上下文指针
    match manager.get_process(next_pid) {
        Some(pcb) => pcb.context() as *const _,
        None => ptr::null(),
    }
}

/// 获取指定进程的上下文指针
#[no_mangle]
pub extern "C" fn rust_get_process_context(pid: ProcessId) -> *const context::ProcessContext {
    let manager = ProcessManager::instance();
    
    match manager.get_process(pid) {
        Some(pcb) => pcb.context() as *const _,
        None => ptr::null(),
    }
}

// Idle进程入口点在汇编中实现
// extern "C" {
//     pub fn idle_process_entry();
// }
