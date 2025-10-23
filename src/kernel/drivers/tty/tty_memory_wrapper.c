// TTY内存管理包装器
// 临时使用简单分配器，直到Rust内存管理器完全实现

#include "kernel/tty.h"

// 简单分配器函数（临时）
extern void* simple_malloc(size_t size);
extern void simple_free(void* ptr);
extern void simple_memory_stats(size_t *total, size_t *used, size_t *free, size_t *peak);

// TTY内存管理函数实现（使用简单分配器）
void* tty_kmalloc(size_t size) {
    return simple_malloc(size);
}

void tty_kfree(void* ptr) {
    simple_free(ptr);
}

void tty_memory_stats(size_t *total, size_t *used, size_t *free, size_t *peak) {
    simple_memory_stats(total, used, free, peak);
}

void* tty_kmalloc_large(size_t size) {
    return simple_malloc(size);
}

void tty_kfree_large(void* ptr) {
    simple_free(ptr);
}

void* tty_map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    (void)virtual_addr;
    (void)physical_addr;
    (void)flags;
    // 暂时不支持页面映射
    return NULL;
}

uint64_t tty_get_physical_addr(uint64_t virtual_addr) {
    // 暂时返回相同地址（假设身份映射）
    return virtual_addr;
}
