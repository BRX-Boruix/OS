//! 进程控制块 (PCB) 实现

#![allow(dead_code)]

use crate::arch::*;
use crate::context::ProcessContext;
use crate::state::{ProcessState, BlockReason};
use crate::scheduler::Priority;
use crate::ProcessId;

/// 进程名称最大长度
const MAX_NAME_LEN: usize = 32;

/// 进程控制块
pub struct ProcessControlBlock {
    /// 进程ID
    pid: ProcessId,
    /// 父进程ID
    parent_pid: ProcessId,
    /// 进程名称
    name: [u8; MAX_NAME_LEN],
    /// 进程状态
    state: ProcessState,
    /// 阻塞原因
    block_reason: BlockReason,
    /// 调度优先级
    priority: Priority,
    /// 进程上下文
    context: ProcessContext,
    /// 内核栈指针
    kernel_stack_ptr: Option<usize>,
    /// 内核栈基址
    kernel_stack_base: Option<usize>,
    /// 用户栈指针
    user_stack_ptr: Option<usize>,
    /// 用户栈基址
    user_stack_base: Option<usize>,
    /// 入口点地址
    entry_point: Option<usize>,
    /// 退出码
    exit_code: Option<i32>,
    /// 时间片
    time_slice: u32,
    /// CPU使用时间
    cpu_time: u64,
    /// 创建时间
    pub created_at: u64,
}

impl ProcessControlBlock {
    /// 创建新的PCB
    pub fn new(pid: ProcessId, name: &str) -> Result<Self, &'static str> {
        let mut name_buf = [0u8; MAX_NAME_LEN];
        let name_bytes = name.as_bytes();
        let len = core::cmp::min(name_bytes.len(), MAX_NAME_LEN - 1);
        name_buf[..len].copy_from_slice(&name_bytes[..len]);

        Ok(Self {
            pid,
            parent_pid: 0,
            name: name_buf,
            state: ProcessState::Created,
            block_reason: BlockReason::None,
            priority: Priority::Normal,
            context: ProcessContext::new(),
            kernel_stack_ptr: None,
            kernel_stack_base: None,
            user_stack_ptr: None,
            user_stack_base: None,
            entry_point: None,
            exit_code: None,
            time_slice: 0,
            cpu_time: 0,
            created_at: read_tsc(),
        })
    }

    /// 获取进程ID
    pub fn pid(&self) -> ProcessId {
        self.pid
    }

    /// 获取父进程ID
    pub fn parent_pid(&self) -> ProcessId {
        self.parent_pid
    }

    /// 设置父进程ID
    pub fn set_parent_pid(&mut self, parent_pid: ProcessId) {
        self.parent_pid = parent_pid;
    }

    /// 获取进程名称
    pub fn name(&self) -> &str {
        let len = self.name.iter().position(|&c| c == 0).unwrap_or(MAX_NAME_LEN);
        core::str::from_utf8(&self.name[..len]).unwrap_or("<invalid>")
    }

    /// 获取进程状态
    pub fn state(&self) -> ProcessState {
        self.state
    }

    /// 设置进程状态
    pub fn set_state(&mut self, state: ProcessState) {
        self.state = state;
    }

    /// 获取阻塞原因
    pub fn block_reason(&self) -> BlockReason {
        self.block_reason
    }

    /// 设置阻塞原因
    pub fn set_block_reason(&mut self, reason: BlockReason) {
        self.block_reason = reason;
    }

    /// 获取优先级
    pub fn priority(&self) -> Priority {
        self.priority
    }

    /// 设置优先级
    pub fn set_priority(&mut self, priority: Priority) {
        self.priority = priority;
    }

    /// 获取上下文
    pub fn context(&self) -> &ProcessContext {
        &self.context
    }

    /// 获取可变上下文
    pub fn context_mut(&mut self) -> &mut ProcessContext {
        &mut self.context
    }

    /// 获取内核栈指针
    pub fn kernel_stack_ptr(&self) -> Option<usize> {
        self.kernel_stack_ptr
    }

    /// 设置内核栈指针
    pub fn set_kernel_stack_ptr(&mut self, ptr: usize) {
        self.kernel_stack_ptr = Some(ptr);
    }

    /// 获取用户栈指针
    pub fn user_stack_ptr(&self) -> Option<usize> {
        self.user_stack_ptr
    }

    /// 设置用户栈指针
    pub fn set_user_stack_ptr(&mut self, ptr: usize) {
        self.user_stack_ptr = Some(ptr);
    }

    /// 设置入口点
    pub fn set_entry_point(&mut self, entry: usize) {
        self.entry_point = Some(entry);
    }

    /// 获取入口点
    pub fn entry_point(&self) -> Option<usize> {
        self.entry_point
    }

    /// 设置退出码
    pub fn set_exit_code(&mut self, code: i32) {
        self.exit_code = Some(code);
    }

    /// 获取退出码
    pub fn exit_code(&self) -> Option<i32> {
        self.exit_code
    }

    /// 获取时间片
    pub fn time_slice(&self) -> u32 {
        self.time_slice
    }

    /// 设置时间片
    pub fn set_time_slice(&mut self, slice: u32) {
        self.time_slice = slice;
    }

    /// 减少时间片
    pub fn decrease_time_slice(&mut self) -> u32 {
        if self.time_slice > 0 {
            self.time_slice -= 1;
        }
        self.time_slice
    }

    /// 获取CPU时间
    pub fn cpu_time(&self) -> u64 {
        self.cpu_time
    }

    /// 增加CPU时间
    pub fn add_cpu_time(&mut self, time: u64) {
        self.cpu_time += time;
    }

    /// 分配内核栈
    pub fn allocate_kernel_stack(&mut self) -> Result<(), &'static str> {
        // 调用内存管理器分配内核栈
        // 这里暂时使用简单的实现，实际应该调用内存管理器
        extern "C" {
            fn rust_allocate_pages(count: usize) -> *mut u8;
        }

        let pages = (KERNEL_STACK_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
        let stack_base = unsafe { rust_allocate_pages(pages) };

        if stack_base.is_null() {
            return Err("Failed to allocate kernel stack");
        }

        let stack_base_addr = stack_base as usize;
        let stack_ptr = stack_base_addr + KERNEL_STACK_SIZE;

        self.kernel_stack_base = Some(stack_base_addr);
        self.kernel_stack_ptr = Some(stack_ptr);

        Ok(())
    }

    /// 分配用户栈
    pub fn allocate_user_stack(&mut self) -> Result<(), &'static str> {
        // 调用内存管理器分配用户栈
        extern "C" {
            fn rust_allocate_pages(count: usize) -> *mut u8;
        }

        let pages = (USER_STACK_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
        let stack_base = unsafe { rust_allocate_pages(pages) };

        if stack_base.is_null() {
            return Err("Failed to allocate user stack");
        }

        let stack_base_addr = stack_base as usize;
        let stack_ptr = stack_base_addr + USER_STACK_SIZE;

        self.user_stack_base = Some(stack_base_addr);
        self.user_stack_ptr = Some(stack_ptr);

        Ok(())
    }

    /// 释放内核栈
    pub fn free_kernel_stack(&mut self) {
        if let Some(base) = self.kernel_stack_base {
            extern "C" {
                fn rust_free_pages(ptr: *mut u8, count: usize);
            }
            let pages = (KERNEL_STACK_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
            unsafe {
                rust_free_pages(base as *mut u8, pages);
            }
            self.kernel_stack_base = None;
            self.kernel_stack_ptr = None;
        }
    }

    /// 释放用户栈
    pub fn free_user_stack(&mut self) {
        if let Some(base) = self.user_stack_base {
            extern "C" {
                fn rust_free_pages(ptr: *mut u8, count: usize);
            }
            let pages = (USER_STACK_SIZE + PAGE_SIZE - 1) / PAGE_SIZE;
            unsafe {
                rust_free_pages(base as *mut u8, pages);
            }
            self.user_stack_base = None;
            self.user_stack_ptr = None;
        }
    }
}

impl Drop for ProcessControlBlock {
    fn drop(&mut self) {
        // 自动释放栈空间
        self.free_kernel_stack();
        self.free_user_stack();
    }
}

