//! 进程调度器实现

#![allow(dead_code)]

use crate::ProcessId;

/// 调度优先级
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum Priority {
    /// 实时优先级（最高）
    Realtime = 0,
    /// 高优先级
    High = 1,
    /// 普通优先级
    Normal = 2,
    /// 低优先级
    Low = 3,
    /// Idle优先级（最低）
    Idle = 4,
}

impl Priority {
    /// 从u8转换为Priority
    pub fn from_u8(value: u8) -> Option<Self> {
        match value {
            0 => Some(Priority::Realtime),
            1 => Some(Priority::High),
            2 => Some(Priority::Normal),
            3 => Some(Priority::Low),
            4 => Some(Priority::Idle),
            _ => None,
        }
    }

    /// 转换为字符串描述
    pub fn as_str(&self) -> &'static str {
        match self {
            Priority::Realtime => "Realtime",
            Priority::High => "High",
            Priority::Normal => "Normal",
            Priority::Low => "Low",
            Priority::Idle => "Idle",
        }
    }

    /// 获取默认时间片（毫秒）
    pub fn default_time_slice(&self) -> u32 {
        match self {
            Priority::Realtime => 20,
            Priority::High => 15,
            Priority::Normal => 10,
            Priority::Low => 5,
            Priority::Idle => 1,
        }
    }
}

/// 调度策略
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SchedulingPolicy {
    /// 轮转调度
    RoundRobin = 0,
    /// 优先级调度
    Priority = 1,
    /// 多级反馈队列
    MultilevelFeedback = 2,
}

/// 调度器统计信息
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct SchedulerStats {
    /// 总调度次数
    pub total_schedules: u64,
    /// 上下文切换次数
    pub context_switches: u64,
    /// 抢占次数
    pub preemptions: u64,
    /// Idle时间
    pub idle_time: u64,
    /// 各优先级调度次数
    pub priority_schedules: [u64; 5],
}

impl SchedulerStats {
    /// 创建新的统计信息
    pub const fn new() -> Self {
        Self {
            total_schedules: 0,
            context_switches: 0,
            preemptions: 0,
            idle_time: 0,
            priority_schedules: [0; 5],
        }
    }
}

/// 进程调度器
pub struct Scheduler {
    /// 调度策略
    policy: SchedulingPolicy,
    /// 当前进程ID
    current_process: Option<ProcessId>,
    /// 统计信息
    stats: SchedulerStats,
    /// 时间片计数器
    time_slice_counter: u32,
    /// 调度器是否启用
    enabled: bool,
}

impl Scheduler {
    /// 创建新的调度器
    pub const fn new() -> Self {
        Self {
            policy: SchedulingPolicy::RoundRobin,
            current_process: None,
            stats: SchedulerStats::new(),
            time_slice_counter: 10,
            enabled: false,
        }
    }

    /// 初始化调度器
    pub fn init(&mut self) -> Result<(), &'static str> {
        self.enabled = false;
        self.time_slice_counter = 10;
        self.current_process = None;
        Ok(())
    }

    /// 启用调度器
    pub fn enable(&mut self) {
        self.enabled = true;
        self.time_slice_counter = 10;
    }

    /// 禁用调度器
    pub fn disable(&mut self) {
        self.enabled = false;
    }

    /// 检查调度器是否启用
    pub fn is_enabled(&self) -> bool {
        self.enabled
    }

    /// 设置调度策略
    pub fn set_policy(&mut self, policy: SchedulingPolicy) {
        self.policy = policy;
    }

    /// 获取调度策略
    pub fn policy(&self) -> SchedulingPolicy {
        self.policy
    }

    /// 获取当前进程ID
    pub fn current_process(&self) -> Option<ProcessId> {
        self.current_process
    }

    /// 设置当前进程ID
    pub fn set_current_process(&mut self, pid: Option<ProcessId>) {
        self.current_process = pid;
    }

    /// 获取统计信息
    pub fn stats(&self) -> &SchedulerStats {
        &self.stats
    }

    /// 时钟中断处理
    pub fn tick(&mut self) -> bool {
        if !self.enabled {
            return false;
        }

        if self.time_slice_counter > 0 {
            self.time_slice_counter -= 1;
        }

        // 检查是否需要抢占
        if self.time_slice_counter == 0 {
            self.stats.preemptions += 1;
            true // 需要重新调度
        } else {
            false
        }
    }

    /// 执行调度
    pub fn schedule(&mut self) -> Result<ProcessId, &'static str> {
        if !self.enabled {
            return Err("Scheduler not enabled");
        }

        self.stats.total_schedules += 1;

        // 简单的轮转调度实现
        // 实际调度逻辑由ProcessManager处理
        match self.current_process {
            Some(pid) => {
                self.time_slice_counter = 10;
                Ok(pid)
            }
            None => {
                // 返回idle进程
                Ok(1)
            }
        }
    }

    /// 进程让出CPU
    pub fn yield_cpu(&mut self) -> Result<ProcessId, &'static str> {
        self.stats.total_schedules += 1;
        self.schedule()
    }

    /// 阻塞当前进程
    pub fn block_current(&mut self) {
        self.current_process = None;
        self.time_slice_counter = 0;
    }

    /// 唤醒进程
    pub fn wakeup_process(&mut self, _pid: ProcessId) {
        // 实际唤醒逻辑由ProcessManager处理
    }

    /// 记录上下文切换
    pub fn record_context_switch(&mut self, from_pid: ProcessId, to_pid: ProcessId) {
        if from_pid != to_pid {
            self.stats.context_switches += 1;
        }
        self.current_process = Some(to_pid);
        self.time_slice_counter = 10;
    }

    /// 强制重新调度（将时间片设为0）
    pub fn force_reschedule(&mut self) {
        self.time_slice_counter = 0;
    }

    /// 记录优先级调度
    pub fn record_priority_schedule(&mut self, priority: Priority) {
        let idx = priority as usize;
        if idx < self.stats.priority_schedules.len() {
            self.stats.priority_schedules[idx] += 1;
        }
    }

    /// 增加idle时间
    pub fn add_idle_time(&mut self, time: u64) {
        self.stats.idle_time += time;
    }
    
    /// 记录调度事件
    pub fn record_schedule(&mut self) {
        self.stats.total_schedules += 1;
    }
}

