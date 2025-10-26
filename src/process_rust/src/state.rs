//! 进程状态定义

#![allow(dead_code)]

/// 进程状态枚举
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ProcessState {
    /// 已创建但未初始化
    Created = 0,
    /// 就绪状态，等待调度
    Ready = 1,
    /// 运行状态
    Running = 2,
    /// 阻塞状态，等待事件
    Blocked = 3,
    /// 僵尸状态，已退出但未清理
    Zombie = 4,
    /// 已终止
    Terminated = 5,
}

impl ProcessState {
    /// 从u8转换为ProcessState
    pub fn from_u8(value: u8) -> Option<Self> {
        match value {
            0 => Some(ProcessState::Created),
            1 => Some(ProcessState::Ready),
            2 => Some(ProcessState::Running),
            3 => Some(ProcessState::Blocked),
            4 => Some(ProcessState::Zombie),
            5 => Some(ProcessState::Terminated),
            _ => None,
        }
    }

    /// 转换为字符串描述
    pub fn as_str(&self) -> &'static str {
        match self {
            ProcessState::Created => "Created",
            ProcessState::Ready => "Ready",
            ProcessState::Running => "Running",
            ProcessState::Blocked => "Blocked",
            ProcessState::Zombie => "Zombie",
            ProcessState::Terminated => "Terminated",
        }
    }

    /// 检查是否可以调度
    pub fn is_schedulable(&self) -> bool {
        matches!(self, ProcessState::Ready | ProcessState::Running)
    }

    /// 检查是否已终止
    pub fn is_terminated(&self) -> bool {
        matches!(self, ProcessState::Zombie | ProcessState::Terminated)
    }
}

/// 阻塞原因
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BlockReason {
    /// 未阻塞
    None = 0,
    /// 等待I/O
    WaitingForIO = 1,
    /// 等待消息
    WaitingForMessage = 2,
    /// 等待信号量
    WaitingForSemaphore = 3,
    /// 等待互斥锁
    WaitingForMutex = 4,
    /// 等待子进程
    WaitingForChild = 5,
    /// 睡眠
    Sleeping = 6,
}

impl BlockReason {
    /// 从u8转换为BlockReason
    pub fn from_u8(value: u8) -> Option<Self> {
        match value {
            0 => Some(BlockReason::None),
            1 => Some(BlockReason::WaitingForIO),
            2 => Some(BlockReason::WaitingForMessage),
            3 => Some(BlockReason::WaitingForSemaphore),
            4 => Some(BlockReason::WaitingForMutex),
            5 => Some(BlockReason::WaitingForChild),
            6 => Some(BlockReason::Sleeping),
            _ => None,
        }
    }

    /// 转换为字符串描述
    pub fn as_str(&self) -> &'static str {
        match self {
            BlockReason::None => "None",
            BlockReason::WaitingForIO => "Waiting for I/O",
            BlockReason::WaitingForMessage => "Waiting for message",
            BlockReason::WaitingForSemaphore => "Waiting for semaphore",
            BlockReason::WaitingForMutex => "Waiting for mutex",
            BlockReason::WaitingForChild => "Waiting for child",
            BlockReason::Sleeping => "Sleeping",
        }
    }
}

