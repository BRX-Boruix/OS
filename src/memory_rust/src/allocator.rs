//! 内存分配器接口
//! 提供统一的内存分配接口

use crate::arch::addr::{PhysAddr, VirtAddr};
use crate::heap::HeapStats;
use crate::physical::PhysFrame;
use core::ptr::NonNull;

/// 分配器错误类型
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AllocatorError {
    /// 内存不足
    OutOfMemory,
    /// 无效的大小
    InvalidSize,
    /// 无效的对齐
    InvalidAlignment,
    /// 无效的指针
    InvalidPointer,
    /// 分配器未初始化
    NotInitialized,
}

/// 分配器结果类型
pub type AllocatorResult<T> = Result<T, AllocatorError>;

/// 内存分配器特征
pub trait Allocator {
    /// 分配内存
    fn allocate(&mut self, size: usize, align: usize) -> AllocatorResult<NonNull<u8>>;

    /// 释放内存
    fn deallocate(&mut self, ptr: NonNull<u8>, size: usize, align: usize) -> AllocatorResult<()>;

    /// 重新分配内存
    fn reallocate(
        &mut self,
        ptr: NonNull<u8>,
        old_size: usize,
        new_size: usize,
        align: usize,
    ) -> AllocatorResult<NonNull<u8>> {
        // 默认实现：分配新内存，复制数据，释放旧内存
        let new_ptr = self.allocate(new_size, align)?;

        unsafe {
            let copy_size = core::cmp::min(old_size, new_size);
            core::ptr::copy_nonoverlapping(ptr.as_ptr(), new_ptr.as_ptr(), copy_size);
        }

        self.deallocate(ptr, old_size, align)?;
        Ok(new_ptr)
    }
}

/// 页面分配器特征
pub trait PageAllocator {
    /// 分配单个页面
    fn allocate_page(&mut self) -> AllocatorResult<PhysFrame>;

    /// 分配连续页面
    fn allocate_pages(&mut self, count: usize) -> AllocatorResult<PhysFrame>;

    /// 释放单个页面
    fn deallocate_page(&mut self, frame: PhysFrame) -> AllocatorResult<()>;

    /// 释放连续页面
    fn deallocate_pages(&mut self, start_frame: PhysFrame, count: usize) -> AllocatorResult<()>;
}

/// 虚拟内存分配器特征
pub trait VirtualAllocator {
    /// 映射页面
    fn map_page(
        &mut self,
        virtual_addr: VirtAddr,
        physical_addr: PhysAddr,
        flags: u64,
    ) -> AllocatorResult<()>;

    /// 取消映射
    fn unmap_page(&mut self, virtual_addr: VirtAddr) -> AllocatorResult<PhysAddr>;

    /// 映射连续页面
    fn map_pages(
        &mut self,
        start_virt: VirtAddr,
        start_phys: PhysAddr,
        count: usize,
        flags: u64,
    ) -> AllocatorResult<()>;

    /// 取消连续页面映射
    fn unmap_pages(&mut self, start_virt: VirtAddr, count: usize) -> AllocatorResult<()>;

    /// 地址转换
    fn translate(&self, virtual_addr: VirtAddr) -> Option<PhysAddr>;
}

/// 统计信息提供者特征
pub trait StatsProvider {
    /// 获取堆统计信息
    fn heap_stats(&self) -> HeapStats;

    /// 获取物理内存统计
    fn physical_stats(&self) -> PhysicalStats;

    /// 获取虚拟内存统计
    fn virtual_stats(&self) -> VirtualStats;
}

/// 物理内存统计
#[derive(Debug, Clone, Copy)]
pub struct PhysicalStats {
    pub total_pages: usize,
    pub allocated_pages: usize,
    pub free_pages: usize,
    pub usage_percent: u32,
}

/// 虚拟内存统计
#[derive(Debug, Clone, Copy)]
pub struct VirtualStats {
    pub mapped_pages: usize,
    pub page_tables_count: usize,
}

/// 通用内存分配函数
pub fn kmalloc(size: usize) -> AllocatorResult<*mut u8> {
    if size == 0 {
        return Err(AllocatorError::InvalidSize);
    }

    // 这里应该调用全局内存管理器
    // 简化实现，返回错误
    Err(AllocatorError::NotInitialized)
}

/// 对齐的内存分配函数
pub fn kmalloc_aligned(size: usize, align: usize) -> AllocatorResult<*mut u8> {
    if size == 0 {
        return Err(AllocatorError::InvalidSize);
    }

    if !align.is_power_of_two() {
        return Err(AllocatorError::InvalidAlignment);
    }

    // 这里应该调用全局内存管理器
    // 简化实现，返回错误
    Err(AllocatorError::NotInitialized)
}

/// 清零的内存分配函数
pub fn kcalloc(count: usize, size: usize) -> AllocatorResult<*mut u8> {
    let total_size = count.checked_mul(size).ok_or(AllocatorError::InvalidSize)?;

    let ptr = kmalloc(total_size)?;

    // 清零内存
    unsafe {
        core::ptr::write_bytes(ptr, 0, total_size);
    }

    Ok(ptr)
}

/// 内存释放函数
pub fn kfree(ptr: *mut u8) -> AllocatorResult<()> {
    if ptr.is_null() {
        return Ok(()); // 释放空指针是安全的
    }

    // 这里应该调用全局内存管理器
    // 简化实现，返回错误
    Err(AllocatorError::NotInitialized)
}

/// 内存重新分配函数
pub fn krealloc(ptr: *mut u8, _old_size: usize, new_size: usize) -> AllocatorResult<*mut u8> {
    if new_size == 0 {
        if !ptr.is_null() {
            kfree(ptr)?;
        }
        return Ok(core::ptr::null_mut());
    }

    if ptr.is_null() {
        return kmalloc(new_size);
    }

    // 这里应该调用全局内存管理器的重新分配功能
    // 简化实现，返回错误
    Err(AllocatorError::NotInitialized)
}

/// 页面分配函数
pub fn alloc_page() -> AllocatorResult<PhysFrame> {
    // 这里应该调用全局内存管理器的页面分配器
    Err(AllocatorError::NotInitialized)
}

/// 连续页面分配函数
pub fn alloc_pages(count: usize) -> AllocatorResult<PhysFrame> {
    if count == 0 {
        return Err(AllocatorError::InvalidSize);
    }

    // 这里应该调用全局内存管理器的页面分配器
    Err(AllocatorError::NotInitialized)
}

/// 页面释放函数
pub fn free_page(_frame: PhysFrame) -> AllocatorResult<()> {
    // 这里应该调用全局内存管理器的页面分配器
    Err(AllocatorError::NotInitialized)
}

/// 连续页面释放函数
pub fn free_pages(_start_frame: PhysFrame, count: usize) -> AllocatorResult<()> {
    if count == 0 {
        return Ok(());
    }

    // 这里应该调用全局内存管理器的页面分配器
    Err(AllocatorError::NotInitialized)
}

/// 页面映射函数
pub fn map_page(virt: VirtAddr, phys: PhysAddr, flags: u64) -> AllocatorResult<()> {
    // 这里应该调用全局内存管理器的页表管理器
    Err(AllocatorError::NotInitialized)
}

/// 页面取消映射函数
pub fn unmap_page(virt: VirtAddr) -> AllocatorResult<PhysAddr> {
    // 这里应该调用全局内存管理器的页表管理器
    Err(AllocatorError::NotInitialized)
}

/// 地址转换函数
pub fn virt_to_phys(virt: VirtAddr) -> Option<PhysAddr> {
    // 这里应该调用全局内存管理器的页表管理器
    None
}

/// 内存统计函数
pub fn memory_stats() -> Option<crate::stats::MemoryStats> {
    // 这里应该调用全局内存管理器获取统计信息
    None
}

/// 内存使用摘要
pub fn memory_summary() -> Option<crate::stats::MemorySummary> {
    memory_stats().map(|stats| stats.summary())
}
