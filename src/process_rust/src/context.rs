//! 进程上下文和上下文切换

#![allow(dead_code)]

use crate::arch::*;

/// 进程上下文结构 (与中断栈帧兼容)
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct ProcessContext {
    // 通用寄存器
    pub rax: u64,
    pub rbx: u64,
    pub rcx: u64,
    pub rdx: u64,
    pub rsi: u64,
    pub rdi: u64,
    pub rbp: u64,
    pub r8: u64,
    pub r9: u64,
    pub r10: u64,
    pub r11: u64,
    pub r12: u64,
    pub r13: u64,
    pub r14: u64,
    pub r15: u64,
    
    // 中断相关
    pub int_no: u64,
    pub err_code: u64,
    
    // CPU自动压栈
    pub rip: u64,
    pub cs: u64,
    pub rflags: u64,
    pub rsp: u64,
    pub ss: u64,
}

impl ProcessContext {
    /// 创建新的空上下文
    pub const fn new() -> Self {
        Self {
            rax: 0,
            rbx: 0,
            rcx: 0,
            rdx: 0,
            rsi: 0,
            rdi: 0,
            rbp: 0,
            r8: 0,
            r9: 0,
            r10: 0,
            r11: 0,
            r12: 0,
            r13: 0,
            r14: 0,
            r15: 0,
            int_no: 0,
            err_code: 0,
            rip: 0,
            cs: KERNEL_CS,
            rflags: DEFAULT_RFLAGS,
            rsp: 0,
            ss: KERNEL_DS,
        }
    }

    /// 初始化内核态上下文
    pub fn init_kernel_context(&mut self, entry_point: usize, stack_ptr: usize) {
        self.rip = entry_point as u64;
        self.rsp = stack_ptr as u64;
        self.cs = KERNEL_CS;
        self.ss = KERNEL_DS;
        self.rflags = DEFAULT_RFLAGS;
        
        // 清空其他寄存器
        self.rax = 0;
        self.rbx = 0;
        self.rcx = 0;
        self.rdx = 0;
        self.rsi = 0;
        self.rdi = 0;
        self.rbp = 0;
        self.r8 = 0;
        self.r9 = 0;
        self.r10 = 0;
        self.r11 = 0;
        self.r12 = 0;
        self.r13 = 0;
        self.r14 = 0;
        self.r15 = 0;
        self.int_no = 0;
        self.err_code = 0;
    }

    /// 初始化用户态上下文
    pub fn init_user_context(&mut self, entry_point: usize, stack_ptr: usize) {
        self.rip = entry_point as u64;
        self.rsp = stack_ptr as u64;
        self.cs = USER_CS;
        self.ss = USER_DS;
        self.rflags = DEFAULT_RFLAGS;
        
        // 清空其他寄存器
        self.rax = 0;
        self.rbx = 0;
        self.rcx = 0;
        self.rdx = 0;
        self.rsi = 0;
        self.rdi = 0;
        self.rbp = 0;
        self.r8 = 0;
        self.r9 = 0;
        self.r10 = 0;
        self.r11 = 0;
        self.r12 = 0;
        self.r13 = 0;
        self.r14 = 0;
        self.r15 = 0;
        self.int_no = 0;
        self.err_code = 0;
    }

    /// 保存当前CPU状态
    pub fn save_current(&mut self) {
        unsafe {
            core::arch::asm!(
                "mov {rax}, rax",
                "mov {rbx}, rbx",
                "mov {rcx}, rcx",
                "mov {rdx}, rdx",
                "mov {rsi}, rsi",
                "mov {rdi}, rdi",
                "mov {rbp}, rbp",
                "mov {r8}, r8",
                "mov {r9}, r9",
                "mov {r10}, r10",
                "mov {r11}, r11",
                "mov {r12}, r12",
                "mov {r13}, r13",
                "mov {r14}, r14",
                "mov {r15}, r15",
                rax = out(reg) self.rax,
                rbx = out(reg) self.rbx,
                rcx = out(reg) self.rcx,
                rdx = out(reg) self.rdx,
                rsi = out(reg) self.rsi,
                rdi = out(reg) self.rdi,
                rbp = out(reg) self.rbp,
                r8 = out(reg) self.r8,
                r9 = out(reg) self.r9,
                r10 = out(reg) self.r10,
                r11 = out(reg) self.r11,
                r12 = out(reg) self.r12,
                r13 = out(reg) self.r13,
                r14 = out(reg) self.r14,
                r15 = out(reg) self.r15,
                options(nomem, nostack)
            );
        }
        self.rsp = get_stack_pointer() as u64;
    }
}

// 上下文切换函数（在汇编中实现）
extern "C" {
    pub fn switch_context(from: *mut ProcessContext, to: *const ProcessContext);
}

