// heap.rs - 内核堆分配器
// 基于VMM实现的动态内存分配（kmalloc/kfree）

use crate::arch::addr::VirtAddr;
use crate::vmm::{VirtualMemoryManager, VmmFlags};
use crate::paging::PageTableManager;
use crate::lazy_buddy::PhysFrame;

/// 堆块头部
#[repr(C)]
struct HeapBlock {
    size: usize,           // 块大小（不包括头部）
    is_free: bool,         // 是否空闲
    next: Option<*mut HeapBlock>,  // 下一个块
}

impl HeapBlock {
    const HEADER_SIZE: usize = core::mem::size_of::<HeapBlock>();

    fn new(size: usize, is_free: bool) -> Self {
        Self {
            size,
            is_free,
            next: None,
        }
    }

    // 获取数据区域指针
    fn data_ptr(&self) -> *mut u8 {
        unsafe {
            (self as *const HeapBlock as *mut u8).add(Self::HEADER_SIZE)
        }
    }

    // 从数据指针获取块头
    unsafe fn from_data_ptr(ptr: *mut u8) -> *mut HeapBlock {
        ptr.sub(Self::HEADER_SIZE) as *mut HeapBlock
    }

    // 分割块
    unsafe fn split(&mut self, size: usize) -> Option<*mut HeapBlock> {
        if self.size < size + Self::HEADER_SIZE + 16 {
            // 剩余空间太小，不分割
            return None;
        }

        let new_size = self.size - size - Self::HEADER_SIZE;
        self.size = size;

        // 创建新块
        let new_block_ptr = self.data_ptr().add(size) as *mut HeapBlock;
        let new_block = HeapBlock::new(new_size, true);
        core::ptr::write(new_block_ptr, new_block);
        
        (*new_block_ptr).next = self.next;
        self.next = Some(new_block_ptr);

        Some(new_block_ptr)
    }

    // 合并相邻的空闲块
    unsafe fn merge_next(&mut self) {
        if let Some(next_ptr) = self.next {
            let next = &mut *next_ptr;
            if next.is_free {
                self.size += Self::HEADER_SIZE + next.size;
                self.next = next.next;
            }
        }
    }
}

/// 堆分配器
pub struct HeapAllocator {
    head: Option<*mut HeapBlock>,  // 链表头
    total_allocated: usize,        // 总分配字节数
    total_freed: usize,            // 总释放字节数
    allocation_count: usize,       // 分配次数
    free_count: usize,             // 释放次数
}

impl HeapAllocator {
    pub const fn new() -> Self {
        Self {
            head: None,
            total_allocated: 0,
            total_freed: 0,
            allocation_count: 0,
            free_count: 0,
        }
    }

    /// 初始化堆分配器
    pub fn init(&mut self) -> Result<(), &'static str> {
        // 堆分配器采用按需分配策略，不需要预先初始化
        Ok(())
    }

    /// 分配内存
    pub fn allocate<F>(
        &mut self,
        size: usize,
        vmm: &mut VirtualMemoryManager,
        page_table: &mut PageTableManager,
        mut alloc_frame: F,
    ) -> Result<*mut u8, &'static str>
    where
        F: FnMut() -> Option<PhysFrame>,
    {
        if size == 0 {
            return Err("Cannot allocate zero bytes");
        }

        // 对齐到8字节
        let aligned_size = (size + 7) & !7;

        // 首先尝试从现有块中分配
        if let Some(ptr) = self.find_free_block(aligned_size) {
            self.total_allocated += aligned_size;
            self.allocation_count += 1;
            return Ok(ptr);
        }

        // 没有合适的空闲块，从VMM分配新内存
        let alloc_size = if aligned_size + HeapBlock::HEADER_SIZE < 4096 {
            4096  // 至少分配一页
        } else {
            (aligned_size + HeapBlock::HEADER_SIZE + 4095) & !4095
        };

        let flags = VmmFlags::new().writable();
        let virt_addr = vmm.allocate_and_map(
            page_table,
            alloc_size as u64,
            flags,
            &mut alloc_frame,
        )?;

        // 创建新块
        let block_ptr = virt_addr.as_u64() as *mut HeapBlock;
        let block_size = alloc_size - HeapBlock::HEADER_SIZE;
        
        unsafe {
            let block = HeapBlock::new(block_size, false);
            core::ptr::write(block_ptr, block);

            // 添加到链表
            (*block_ptr).next = self.head;
            self.head = Some(block_ptr);

            // 如果分配的空间比请求的大，分割块
            (*block_ptr).split(aligned_size);

            self.total_allocated += aligned_size;
            self.allocation_count += 1;

            Ok((*block_ptr).data_ptr())
        }
    }

    /// 释放内存
    pub fn deallocate(&mut self, ptr: *mut u8) -> Result<(), &'static str> {
        if ptr.is_null() {
            return Ok(());
        }

        unsafe {
            let block_ptr = HeapBlock::from_data_ptr(ptr);
            let block = &mut *block_ptr;

            if block.is_free {
                return Err("Double free detected");
            }

            block.is_free = true;
            self.total_freed += block.size;
            self.free_count += 1;

            // 尝试合并相邻的空闲块
            block.merge_next();

            // 向前合并
            self.merge_forward(block_ptr);
        }

        Ok(())
    }

    /// 查找合适的空闲块
    fn find_free_block(&mut self, size: usize) -> Option<*mut u8> {
        let mut current = self.head;

        while let Some(block_ptr) = current {
            unsafe {
                let block = &mut *block_ptr;
                if block.is_free && block.size >= size {
                    block.is_free = false;
                    block.split(size);
                    return Some(block.data_ptr());
                }
                current = block.next;
            }
        }

        None
    }

    /// 向前合并空闲块
    unsafe fn merge_forward(&mut self, target_ptr: *mut HeapBlock) {
        let mut current = self.head;

        while let Some(block_ptr) = current {
            let block = &mut *block_ptr;
            if block.is_free && block.next == Some(target_ptr) {
                block.merge_next();
                return;
            }
            current = block.next;
        }
    }

    /// 获取统计信息
    pub fn stats(&self) -> HeapStats {
        HeapStats {
            total_allocated: self.total_allocated,
            total_freed: self.total_freed,
            current_usage: self.total_allocated - self.total_freed,
            allocation_count: self.allocation_count,
            free_count: self.free_count,
        }
    }
}

/// 堆统计信息
#[derive(Debug, Clone, Copy)]
pub struct HeapStats {
    pub total_allocated: usize,
    pub total_freed: usize,
    pub current_usage: usize,
    pub allocation_count: usize,
    pub free_count: usize,
}

