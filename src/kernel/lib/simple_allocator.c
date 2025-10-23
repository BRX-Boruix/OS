// 简单的Bump分配器实现
// 作为Rust内存管理器完成之前的临时方案

#include "kernel/types.h"

// 简单的内存池（8MB）
#define SIMPLE_HEAP_SIZE (8 * 1024 * 1024)
static uint8_t simple_heap[SIMPLE_HEAP_SIZE] __attribute__((aligned(16)));
static size_t heap_offset = 0;

// 简单的内存分配
void* simple_malloc(size_t size) {
    if (size == 0) return NULL;
    
    // 16字节对齐
    size = (size + 15) & ~15;
    
    if (heap_offset + size > SIMPLE_HEAP_SIZE) {
        return NULL;  // 内存耗尽
    }
    
    void* ptr = &simple_heap[heap_offset];
    heap_offset += size;
    
    return ptr;
}

// 简单的内存释放（不做任何事）
void simple_free(void* ptr) {
    (void)ptr;
    // Bump allocator不支持释放
}

// 获取内存使用情况
void simple_memory_stats(size_t *total, size_t *used, size_t *free, size_t *peak) {
    if (total) *total = SIMPLE_HEAP_SIZE;
    if (used) *used = heap_offset;
    if (free) *free = SIMPLE_HEAP_SIZE - heap_offset;
    if (peak) *peak = heap_offset;
}
