//! Boruix OS 进程管理系统 - Rust实现
//! 完整的进程管理，包括PCB、调度器、上下文切换和IPC

#![no_std]
#![no_main]

use core::panic::PanicInfo;
use heapless::Vec;

pub mod arch;
pub mod ffi;
pub mod pcb;
pub mod scheduler;
pub mod context;
pub mod ipc;
pub mod queue;
pub mod state;

// 导出主要接口
pub use pcb::*;
pub use scheduler::*;
pub use context::*;
pub use ipc::*;
pub use queue::*;
pub use state::*;

/// 全局进程管理器实例
static mut PROCESS_MANAGER: Option<ProcessManager> = None;

/// 进程管理器主结构
pub struct ProcessManager {
    scheduler: scheduler::Scheduler,
    current_process: Option<ProcessId>,
    next_pid: ProcessId,
    process_table: Vec<Option<pcb::ProcessControlBlock>, MAX_PROCESSES>,
    ready_queue: queue::ProcessQueue,
    blocked_queue: queue::ProcessQueue,
    zombie_queue: queue::ProcessQueue,
}

/// 最大进程数量
pub const MAX_PROCESSES: usize = 256;

/// 进程ID类型
pub type ProcessId = u32;

/// 无效进程ID
pub const INVALID_PID: ProcessId = 0;

impl ProcessManager {
    /// 创建新的进程管理器
    pub const fn new() -> Self {
        Self {
            scheduler: scheduler::Scheduler::new(),
            current_process: None,
            next_pid: 2,  // 从PID 2开始，PID 0和1是特殊进程
            process_table: Vec::new(),
            ready_queue: queue::ProcessQueue::new(),
            blocked_queue: queue::ProcessQueue::new(),
            zombie_queue: queue::ProcessQueue::new(),
        }
    }

    /// 初始化进程管理器
    pub fn init(&mut self) -> Result<(), &'static str> {
        // 初始化调度器
        self.scheduler.init()?;
        
        // 创建kernel进程（PID 0）- 代表内核本身
        self.create_kernel_process()?;
        
        // 创建idle进程（PID 1）
        self.create_idle_process()?;
        
        Ok(())
    }
    
    /// 创建kernel进程（PID 0）
    fn create_kernel_process(&mut self) -> Result<ProcessId, &'static str> {
        // 强制分配PID 0
        let kernel_pid = 0;
        let mut kernel_pcb = pcb::ProcessControlBlock::new(kernel_pid, "kernel")?;
        
        // 设置kernel进程状态为运行中（它代表内核本身）
        kernel_pcb.set_state(state::ProcessState::Running);
        kernel_pcb.set_priority(scheduler::Priority::Realtime);
        
        // kernel进程不需要栈，因为它代表内核上下文
        // 不分配栈，不设置入口点
        
        // 扩展进程表
        while self.process_table.len() <= kernel_pid as usize {
            self.process_table.push(None).map_err(|_| "Process table full")?;
        }
        
        // 添加到进程表
        self.process_table[kernel_pid as usize] = Some(kernel_pcb);
        
        // kernel进程不加入就绪队列，它始终在运行
        
        Ok(kernel_pid)
    }

    /// 创建idle进程
    fn create_idle_process(&mut self) -> Result<ProcessId, &'static str> {
        // 强制使用PID 1
        let idle_pid = 1;
        let mut idle_pcb = pcb::ProcessControlBlock::new(idle_pid, "idle")?;
        
        // 设置idle进程状态
        idle_pcb.set_state(state::ProcessState::Ready);
        idle_pcb.set_priority(scheduler::Priority::Idle);
        
        // 分配内核栈
        idle_pcb.allocate_kernel_stack()?;
        
        // 设置idle进程入口点（在汇编中实现）
        extern "C" {
            fn idle_process_entry();
        }
        let entry = idle_process_entry as usize;
        idle_pcb.set_entry_point(entry);
        
        // 初始化上下文
        if let Some(stack_ptr) = idle_pcb.kernel_stack_ptr() {
            idle_pcb.context_mut().init_kernel_context(entry, stack_ptr);
        } else {
            return Err("Failed to get idle process stack pointer");
        }
        
        // 扩展进程表到所需大小
        while self.process_table.len() <= idle_pid as usize {
            self.process_table.push(None).map_err(|_| "Process table full")?;
        }
        
        // 添加到进程表
        self.process_table[idle_pid as usize] = Some(idle_pcb);
        
        // 添加到就绪队列
        let _ = self.ready_queue.enqueue(idle_pid);
        
        Ok(idle_pid)
    }

    /// 分配新的PID
    fn allocate_pid(&mut self) -> Result<ProcessId, &'static str> {
        let start_pid = self.next_pid;
        
        loop {
            if self.next_pid >= MAX_PROCESSES as ProcessId {
                self.next_pid = 2; // 跳过PID 0(kernel)和PID 1(idle)
            }
            
            if self.next_pid as usize >= self.process_table.len() || self.process_table[self.next_pid as usize].is_none() {
                let pid = self.next_pid;
                self.next_pid += 1;
                return Ok(pid);
            }
            
            self.next_pid += 1;
            
            // 防止无限循环
            if self.next_pid == start_pid {
                return Err("No available PID");
            }
        }
    }

    /// 获取全局进程管理器实例
    pub fn instance() -> &'static mut Self {
        unsafe {
            PROCESS_MANAGER.as_mut().unwrap_or_else(|| {
                panic!("Process manager not initialized");
            })
        }
    }

    /// 设置全局进程管理器实例
    pub fn set_instance(manager: ProcessManager) {
        unsafe {
            PROCESS_MANAGER = Some(manager);
        }
    }

    /// 获取当前进程ID
    pub fn current_pid(&self) -> Option<ProcessId> {
        self.current_process
    }

    /// 获取进程PCB
    pub fn get_process(&self, pid: ProcessId) -> Option<&pcb::ProcessControlBlock> {
        if pid < MAX_PROCESSES as ProcessId && (pid as usize) < self.process_table.len() {
            self.process_table[pid as usize].as_ref()
        } else {
            None
        }
    }

    /// 获取可变进程PCB
    pub fn get_process_mut(&mut self, pid: ProcessId) -> Option<&mut pcb::ProcessControlBlock> {
        if pid < MAX_PROCESSES as ProcessId && (pid as usize) < self.process_table.len() {
            self.process_table[pid as usize].as_mut()
        } else {
            None
        }
    }
}

/// Panic处理函数
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    // 调用C的串口输出和崩溃触发函数
    extern "C" {
        fn serial_puts(s: *const u8);
        fn trigger_double_fault() -> !;
    }
    
    unsafe {
        // 禁用中断
        core::arch::asm!("cli");
        
        // 输出panic信息
        serial_puts(b"\n\n[RUST PANIC] ========================================\n\0".as_ptr());
        serial_puts(b"[RUST PANIC] CRITICAL SYSTEM PROCESS TERMINATED!\n\0".as_ptr());
        serial_puts(b"[RUST PANIC] ========================================\n\0".as_ptr());
        
        if let Some(location) = info.location() {
            serial_puts(b"[RUST PANIC] Location: \0".as_ptr());
            serial_puts(location.file().as_ptr());
            serial_puts(b":\0".as_ptr());
        }
        
        serial_puts(b"[RUST PANIC] Triggering DOUBLE FAULT...\n\0".as_ptr());
        serial_puts(b"[RUST PANIC] This will display BORUIX KERNEL PANIC page.\n\0".as_ptr());
        serial_puts(b"[RUST PANIC] ========================================\n\n\0".as_ptr());
        
        // 调用C函数触发双重错误
        // 这会显示最高级别的"BORUIX KERNEL PANIC"页面
        trigger_double_fault();
    }
}
