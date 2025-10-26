//! 进程间通信 (IPC) 实现

#![allow(dead_code)]

use crate::ProcessId;
use heapless::Vec;

/// IPC对象ID类型
pub type IpcId = u32;

/// 无效IPC ID
pub const INVALID_IPC_ID: IpcId = 0;

/// 消息最大数据长度
const MAX_MESSAGE_DATA: usize = 256;

/// 消息队列最大容量
const MAX_MESSAGE_QUEUE: usize = 32;

/// 消息类型
#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MessageType {
    /// 普通消息
    Normal = 0,
    /// 紧急消息
    Urgent = 1,
    /// 请求消息
    Request = 2,
    /// 响应消息
    Response = 3,
    /// 信号消息
    Signal = 4,
}

impl MessageType {
    /// 从u32转换为MessageType
    pub fn from_u32(value: u32) -> Option<Self> {
        match value {
            0 => Some(MessageType::Normal),
            1 => Some(MessageType::Urgent),
            2 => Some(MessageType::Request),
            3 => Some(MessageType::Response),
            4 => Some(MessageType::Signal),
            _ => None,
        }
    }
}

/// 消息结构
#[derive(Debug, Clone)]
pub struct Message {
    /// 发送者进程ID
    sender: ProcessId,
    /// 接收者进程ID
    receiver: ProcessId,
    /// 消息类型
    msg_type: u32,
    /// 消息数据
    data: Vec<u8, MAX_MESSAGE_DATA>,
    /// 时间戳
    timestamp: u64,
}

impl Message {
    /// 创建新消息
    pub fn new(
        sender: ProcessId,
        receiver: ProcessId,
        msg_type: u32,
        data: &[u8],
    ) -> Result<Self, &'static str> {
        let mut msg_data = Vec::new();
        msg_data.extend_from_slice(data).map_err(|_| "Message data too large")?;

        Ok(Self {
            sender,
            receiver,
            msg_type,
            data: msg_data,
            timestamp: crate::arch::read_tsc(),
        })
    }

    /// 获取发送者
    pub fn sender(&self) -> ProcessId {
        self.sender
    }

    /// 获取接收者
    pub fn receiver(&self) -> ProcessId {
        self.receiver
    }

    /// 获取消息类型
    pub fn msg_type(&self) -> u32 {
        self.msg_type
    }

    /// 获取消息数据
    pub fn data(&self) -> &[u8] {
        &self.data
    }

    /// 获取时间戳
    pub fn timestamp(&self) -> u64 {
        self.timestamp
    }
}

/// 消息队列
pub struct MessageQueue {
    /// 队列ID
    id: IpcId,
    /// 所有者进程ID
    owner: ProcessId,
    /// 消息列表
    messages: Vec<Message, MAX_MESSAGE_QUEUE>,
}

impl MessageQueue {
    /// 创建新的消息队列
    pub fn new(id: IpcId, owner: ProcessId) -> Self {
        Self {
            id,
            owner,
            messages: Vec::new(),
        }
    }

    /// 获取队列ID
    pub fn id(&self) -> IpcId {
        self.id
    }

    /// 获取所有者
    pub fn owner(&self) -> ProcessId {
        self.owner
    }

    /// 发送消息
    pub fn send(&mut self, message: Message) -> Result<(), &'static str> {
        self.messages.push(message).map_err(|_| "Message queue full")
    }

    /// 接收消息
    pub fn receive(&mut self) -> Option<Message> {
        if self.messages.is_empty() {
            None
        } else {
            Some(self.messages.remove(0))
        }
    }

    /// 查看队首消息
    pub fn peek(&self) -> Option<&Message> {
        self.messages.first()
    }

    /// 检查队列是否为空
    pub fn is_empty(&self) -> bool {
        self.messages.is_empty()
    }

    /// 检查队列是否已满
    pub fn is_full(&self) -> bool {
        self.messages.len() >= MAX_MESSAGE_QUEUE
    }

    /// 获取队列长度
    pub fn len(&self) -> usize {
        self.messages.len()
    }

    /// 清空队列
    pub fn clear(&mut self) {
        self.messages.clear();
    }
}

/// 信号量
pub struct Semaphore {
    /// 信号量ID
    id: IpcId,
    /// 计数值
    count: i32,
    /// 等待队列
    waiting: Vec<ProcessId, 32>,
}

impl Semaphore {
    /// 创建新的信号量
    pub fn new(id: IpcId, initial_count: i32) -> Self {
        Self {
            id,
            count: initial_count,
            waiting: Vec::new(),
        }
    }

    /// 获取信号量ID
    pub fn id(&self) -> IpcId {
        self.id
    }

    /// P操作（等待）
    pub fn wait(&mut self, pid: ProcessId) -> Result<(), &'static str> {
        if self.count > 0 {
            self.count -= 1;
            Ok(())
        } else {
            self.waiting.push(pid).map_err(|_| "Too many waiting processes")
        }
    }

    /// V操作（信号）
    pub fn signal(&mut self) -> Option<ProcessId> {
        if self.waiting.is_empty() {
            self.count += 1;
            None
        } else {
            Some(self.waiting.remove(0))
        }
    }

    /// 获取计数值
    pub fn count(&self) -> i32 {
        self.count
    }

    /// 获取等待进程数
    pub fn waiting_count(&self) -> usize {
        self.waiting.len()
    }
}

/// 互斥锁
pub struct Mutex {
    /// 互斥锁ID
    id: IpcId,
    /// 锁持有者
    owner: Option<ProcessId>,
    /// 等待队列
    waiting: Vec<ProcessId, 32>,
}

impl Mutex {
    /// 创建新的互斥锁
    pub fn new(id: IpcId) -> Self {
        Self {
            id,
            owner: None,
            waiting: Vec::new(),
        }
    }

    /// 获取互斥锁ID
    pub fn id(&self) -> IpcId {
        self.id
    }

    /// 获取锁
    pub fn lock(&mut self, pid: ProcessId) -> Result<(), &'static str> {
        if self.owner.is_none() {
            self.owner = Some(pid);
            Ok(())
        } else if self.owner == Some(pid) {
            Err("Already locked by this process")
        } else {
            self.waiting.push(pid).map_err(|_| "Too many waiting processes")
        }
    }

    /// 释放锁
    pub fn unlock(&mut self, pid: ProcessId) -> Result<Option<ProcessId>, &'static str> {
        if self.owner != Some(pid) {
            return Err("Not the lock owner");
        }

        if self.waiting.is_empty() {
            self.owner = None;
            Ok(None)
        } else {
            let next_owner = self.waiting.remove(0);
            self.owner = Some(next_owner);
            Ok(Some(next_owner))
        }
    }

    /// 检查是否被锁定
    pub fn is_locked(&self) -> bool {
        self.owner.is_some()
    }

    /// 获取锁持有者
    pub fn owner(&self) -> Option<ProcessId> {
        self.owner
    }

    /// 获取等待进程数
    pub fn waiting_count(&self) -> usize {
        self.waiting.len()
    }
}

