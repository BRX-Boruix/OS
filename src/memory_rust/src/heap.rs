//! 内核堆分配器
//! 实现动态内存分配，支持kmalloc和kfree

use crate::arch::addr::{PhysAddr, VirtAddr};
use crate::arch::{KERNEL_HEAP_END, KERNEL_HEAP_START, PAGE_PRESENT, PAGE_SIZE, PAGE_WRITABLE};
use crate::paging::PageTableManager;
use crate::physical::PhysicalAllocator;
use core::alloc::{GlobalAlloc, Layout};
use core::ptr::NonNull;

/// 堆块头部结构
#[repr(C)]
struct HeapBlock {
    size: usize,
    is_free: bool,
    next: Option<NonNull<HeapBlock>>,
    prev: Option<NonNull<HeapBlock>>,
}

impl HeapBlock {
    /// 创建新的堆块
    fn new(size: usize, is_free: bool) -> Self {
        Self {
            size,
            is_free,
            next: None,
            prev: None,
        }
    }

    /// 获取数据区域指针
    fn data_ptr(&self) -> *mut u8 {
        unsafe { (self as *const Self as *mut u8).add(core::mem::size_of::<Self>()) }
    }

    /// 从数据指针获取块头
    unsafe fn from_data_ptr(ptr: *mut u8) -> *mut Self {
        ptr.sub(core::mem::size_of::<Self>()) as *mut Self
    }

    /// 分割块
    fn split(&mut self, size: usize) -> Option<NonNull<HeapBlock>> {
        let block_header_size = core::mem::size_of::<HeapBlock>();
        let min_block_size = 32; // 最小块大小

        if self.size > size + block_header_size + min_block_size {
            // 计算新块的位置
            let new_block_ptr = unsafe {
                (self as *mut Self as *mut u8).add(block_header_size + size) as *mut HeapBlock
            };

            unsafe {
                // 创建新块
                let new_block = &mut *new_block_ptr;
                *new_block = HeapBlock::new(self.size - size - block_header_size, true);

                // 更新链表连接
                new_block.next = self.next;
                new_block.prev = NonNull::new(self as *mut Self);

                if let Some(mut next) = self.next {
                    next.as_mut().prev = NonNull::new(new_block_ptr);
                }

                self.next = NonNull::new(new_block_ptr);
                self.size = size;

                NonNull::new(new_block_ptr)
            }
        } else {
            None
        }
    }

    /// 合并相邻的空闲块
    fn merge_with_next(&mut self) {
        if let Some(next_ptr) = self.next {
            unsafe {
                let next_block = next_ptr.as_ref();
                if next_block.is_free {
                    // 合并块
                    self.size += next_block.size + core::mem::size_of::<HeapBlock>();
                    self.next = next_block.next;

                    if let Some(mut next_next) = next_block.next {
                        next_next.as_mut().prev = NonNull::new(self as *mut Self);
                    }
                }
            }
        }
    }
}

/// 内核堆分配器
pub struct KernelHeap {
    /// 堆起始地址
    heap_start: VirtAddr,
    /// 堆结束地址
    heap_end: VirtAddr,
    /// 当前堆大小
    heap_size: usize,
    /// 空闲块链表头
    free_list: Option<NonNull<HeapBlock>>,
    /// 已分配块链表头
    allocated_list: Option<NonNull<HeapBlock>>,
    /// 总分配字节数
    total_allocated: usize,
    /// 总释放字节数
    total_freed: usize,
    /// 当前使用字节数
    current_usage: usize,
    /// 峰值使用字节数
    peak_usage: usize,
    /// 是否已初始化
    initialized: bool,
}

impl KernelHeap {
    /// 创建新的内核堆
    pub const fn new() -> Self {
        Self {
            heap_start: VirtAddr::new(KERNEL_HEAP_START),
            heap_end: VirtAddr::new(KERNEL_HEAP_END),
            heap_size: 0,
            free_list: None,
            allocated_list: None,
            total_allocated: 0,
            total_freed: 0,
            current_usage: 0,
            peak_usage: 0,
            initialized: false,
        }
    }

    /// 初始化内核堆
    pub fn init(
        &mut self,
        allocator: &mut PhysicalAllocator,
        page_manager: &mut PageTableManager,
    ) -> Result<(), &'static str> {
        if self.initialized {
            return Err("Kernel heap already initialized");
        }

        // 初始堆大小（1MB）
        let initial_heap_size = 1024 * 1024;
        let pages_needed = (initial_heap_size + PAGE_SIZE - 1) / PAGE_SIZE;

        // 分配物理页面并映射到虚拟地址
        for i in 0..pages_needed {
            let frame = allocator
                .allocate_frame()
                .ok_or("Failed to allocate frame for heap")?;

            let virt_addr = VirtAddr::new(self.heap_start.as_u64() + i as u64 * PAGE_SIZE as u64);
            let phys_addr = frame.start_address();

            page_manager.map_page(
                virt_addr,
                phys_addr,
                PAGE_PRESENT | PAGE_WRITABLE,
                allocator,
            )?;
        }

        self.heap_size = pages_needed * PAGE_SIZE;

        // 初始化空闲块链表
        self.init_free_list();

        self.initialized = true;
        Ok(())
    }

    /// 初始化空闲块链表
    fn init_free_list(&mut self) {
        let heap_ptr = self.heap_start.as_u64() as *mut HeapBlock;

        unsafe {
            let initial_block = &mut *heap_ptr;
            *initial_block =
                HeapBlock::new(self.heap_size - core::mem::size_of::<HeapBlock>(), true);

            self.free_list = NonNull::new(heap_ptr);
        }
    }

    /// 分配内存
    pub fn allocate(&mut self, size: usize, align: usize) -> Option<NonNull<u8>> {
        if !self.initialized || size == 0 {
            return None;
        }

        // 对齐大小
        let aligned_size = self.align_size(size, align);

        // 查找合适的空闲块
        if let Some(mut block_ptr) = self.find_free_block(aligned_size) {
            unsafe {
                let block = block_ptr.as_mut();

                // 从空闲链表移除
                self.remove_from_free_list(block_ptr);

                // 分割块（如果需要）
                block.split(aligned_size);

                // 标记为已分配
                block.is_free = false;

                // 添加到已分配链表
                self.add_to_allocated_list(block_ptr);

                // 更新统计信息
                self.total_allocated += aligned_size;
                self.current_usage += aligned_size;
                if self.current_usage > self.peak_usage {
                    self.peak_usage = self.current_usage;
                }

                Some(NonNull::new_unchecked(block.data_ptr()))
            }
        } else {
            // 尝试扩展堆
            self.expand_heap(aligned_size)
                .and_then(|_| self.allocate(size, align))
        }
    }

    /// 释放内存
    pub fn deallocate(&mut self, ptr: NonNull<u8>) {
        if !self.initialized {
            return;
        }

        unsafe {
            let block_ptr = HeapBlock::from_data_ptr(ptr.as_ptr());
            let block = &mut *block_ptr;

            if block.is_free {
                return; // 已经释放
            }

            // 从已分配链表移除
            self.remove_from_allocated_list(NonNull::new_unchecked(block_ptr));

            // 标记为空闲
            block.is_free = true;

            // 更新统计信息
            self.total_freed += block.size;
            self.current_usage -= block.size;

            // 合并相邻的空闲块
            self.merge_free_blocks(NonNull::new_unchecked(block_ptr));

            // 添加到空闲链表
            self.add_to_free_list(NonNull::new_unchecked(block_ptr));
        }
    }

    /// 查找合适的空闲块
    fn find_free_block(&self, size: usize) -> Option<NonNull<HeapBlock>> {
        let mut current = self.free_list;

        while let Some(block_ptr) = current {
            unsafe {
                let block = block_ptr.as_ref();
                if block.is_free && block.size >= size {
                    return Some(block_ptr);
                }
                current = block.next;
            }
        }

        None
    }

    /// 对齐大小
    fn align_size(&self, size: usize, align: usize) -> usize {
        (size + align - 1) & !(align - 1)
    }

    /// 从空闲链表移除块
    fn remove_from_free_list(&mut self, block_ptr: NonNull<HeapBlock>) {
        unsafe {
            let block = block_ptr.as_ref();

            // 更新前一个块的next指针
            if let Some(mut prev_ptr) = block.prev {
                prev_ptr.as_mut().next = block.next;
            } else {
                // 这是头节点
                self.free_list = block.next;
            }

            // 更新后一个块的prev指针
            if let Some(mut next_ptr) = block.next {
                next_ptr.as_mut().prev = block.prev;
            }
        }
    }

    /// 添加到空闲链表
    fn add_to_free_list(&mut self, mut block_ptr: NonNull<HeapBlock>) {
        unsafe {
            let block = block_ptr.as_mut();

            block.next = self.free_list;
            block.prev = None;

            if let Some(mut head_ptr) = self.free_list {
                head_ptr.as_mut().prev = Some(block_ptr);
            }

            self.free_list = Some(block_ptr);
        }
    }

    /// 从已分配链表移除块
    fn remove_from_allocated_list(&mut self, block_ptr: NonNull<HeapBlock>) {
        unsafe {
            let block = block_ptr.as_ref();

            // 更新前一个块的next指针
            if let Some(mut prev_ptr) = block.prev {
                prev_ptr.as_mut().next = block.next;
            } else {
                // 这是头节点
                self.allocated_list = block.next;
            }

            // 更新后一个块的prev指针
            if let Some(mut next_ptr) = block.next {
                next_ptr.as_mut().prev = block.prev;
            }
        }
    }

    /// 添加到已分配链表
    fn add_to_allocated_list(&mut self, mut block_ptr: NonNull<HeapBlock>) {
        unsafe {
            let block = block_ptr.as_mut();

            block.next = self.allocated_list;
            block.prev = None;

            if let Some(mut head_ptr) = self.allocated_list {
                head_ptr.as_mut().prev = Some(block_ptr);
            }

            self.allocated_list = Some(block_ptr);
        }
    }

    /// 合并空闲块
    fn merge_free_blocks(&mut self, mut block_ptr: NonNull<HeapBlock>) {
        unsafe {
            let block = block_ptr.as_mut();

            // 向后合并
            block.merge_with_next();

            // 向前合并
            if let Some(mut prev_ptr) = block.prev {
                let prev_block = prev_ptr.as_mut();
                if prev_block.is_free {
                    prev_block.merge_with_next();
                }
            }
        }
    }

    /// 扩展堆
    fn expand_heap(&mut self, _min_size: usize) -> Option<()> {
        // 简化实现：暂不支持动态扩展
        None
    }

    /// 获取堆统计信息
    pub fn stats(&self) -> HeapStats {
        HeapStats {
            total_size: self.heap_size,
            allocated: self.current_usage,
            free: self.heap_size - self.current_usage,
            peak_usage: self.peak_usage,
            total_allocated: self.total_allocated,
            total_freed: self.total_freed,
        }
    }
}

/// 堆统计信息
#[derive(Debug, Clone, Copy)]
pub struct HeapStats {
    pub total_size: usize,
    pub allocated: usize,
    pub free: usize,
    pub peak_usage: usize,
    pub total_allocated: usize,
    pub total_freed: usize,
}

/// 全局分配器实现
pub struct GlobalKernelAllocator;

unsafe impl GlobalAlloc for GlobalKernelAllocator {
    unsafe fn alloc(&self, _layout: Layout) -> *mut u8 {
        // 这里需要访问全局内存管理器实例
        // 简化实现，返回null
        core::ptr::null_mut()
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        // 这里需要访问全局内存管理器实例
        // 简化实现，什么都不做
    }
}

/// 简化的kmalloc实现
pub fn kmalloc(_size: usize) -> Option<*mut u8> {
    // 这里应该调用全局内存管理器的堆分配器
    // 简化实现，返回None
    None
}

/// 简化的kfree实现
pub fn kfree(_ptr: *mut u8) {
    // 这里应该调用全局内存管理器的堆分配器
    // 简化实现，什么都不做
}
