// Boruix OS TTY专用内存管理
// 简化的内存分配器，专门为TTY系统服务

#include "kernel/memory.h"

// 简单的内存池
#define TTY_MEMORY_POOL_SIZE (64 * 1024)  // 64KB
static char tty_memory_pool[TTY_MEMORY_POOL_SIZE];
static size_t tty_memory_offset = 0;

// TTY专用的内存分配函数
void* tty_kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    // 对齐到8字节
    size = (size + 7) & ~7;
    
    // 检查是否有足够空间
    if (tty_memory_offset + size > TTY_MEMORY_POOL_SIZE) {
        return NULL;  // 内存不足
    }
    
    // 分配内存
    void* ptr = &tty_memory_pool[tty_memory_offset];
    tty_memory_offset += size;
    
    return ptr;
}

// TTY专用的内存释放函数（简化版，不实际释放）
void tty_kfree(void* ptr) {
    // 简化版：不实际释放内存
    // 在实际系统中，这应该实现真正的内存管理
    (void)ptr;
}

// 初始化TTY内存管理
void tty_memory_init(void) {
    tty_memory_offset = 0;
}
