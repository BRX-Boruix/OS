//! 进程队列实现

#![allow(dead_code)]

use crate::ProcessId;
use heapless::Vec;

/// 进程队列最大容量
const MAX_QUEUE_SIZE: usize = 256;

/// 进程队列（环形缓冲区实现）
pub struct ProcessQueue {
    data: Vec<ProcessId, MAX_QUEUE_SIZE>,
}

impl ProcessQueue {
    /// 创建新的进程队列
    pub const fn new() -> Self {
        Self {
            data: Vec::new(),
        }
    }

    /// 入队
    pub fn enqueue(&mut self, pid: ProcessId) -> Result<(), &'static str> {
        self.data.push(pid).map_err(|_| "Queue full")
    }

    /// 出队
    pub fn dequeue(&mut self) -> Option<ProcessId> {
        if self.data.is_empty() {
            None
        } else {
            Some(self.data.remove(0))
        }
    }

    /// 查看队首元素
    pub fn peek(&self) -> Option<ProcessId> {
        self.data.first().copied()
    }

    /// 检查队列是否为空
    pub fn is_empty(&self) -> bool {
        self.data.is_empty()
    }

    /// 检查队列是否已满
    pub fn is_full(&self) -> bool {
        self.data.len() >= MAX_QUEUE_SIZE
    }

    /// 获取队列长度
    pub fn len(&self) -> usize {
        self.data.len()
    }

    /// 清空队列
    pub fn clear(&mut self) {
        self.data.clear();
    }

    /// 检查是否包含指定进程
    pub fn contains(&self, pid: ProcessId) -> bool {
        self.data.iter().any(|&p| p == pid)
    }

    /// 移除指定进程
    pub fn remove(&mut self, pid: ProcessId) -> bool {
        if let Some(pos) = self.data.iter().position(|&p| p == pid) {
            self.data.remove(pos);
            true
        } else {
            false
        }
    }

    /// 获取所有进程ID
    pub fn iter(&self) -> impl Iterator<Item = &ProcessId> {
        self.data.iter()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_queue_basic() {
        let mut queue = ProcessQueue::new();
        assert!(queue.is_empty());
        
        queue.enqueue(1).unwrap();
        queue.enqueue(2).unwrap();
        assert_eq!(queue.len(), 2);
        
        assert_eq!(queue.dequeue(), Some(1));
        assert_eq!(queue.dequeue(), Some(2));
        assert!(queue.is_empty());
    }

    #[test]
    fn test_queue_contains() {
        let mut queue = ProcessQueue::new();
        queue.enqueue(1).unwrap();
        queue.enqueue(2).unwrap();
        
        assert!(queue.contains(1));
        assert!(queue.contains(2));
        assert!(!queue.contains(3));
    }

    #[test]
    fn test_queue_remove() {
        let mut queue = ProcessQueue::new();
        queue.enqueue(1).unwrap();
        queue.enqueue(2).unwrap();
        queue.enqueue(3).unwrap();
        
        assert!(queue.remove(2));
        assert_eq!(queue.len(), 2);
        assert!(!queue.contains(2));
    }
}

