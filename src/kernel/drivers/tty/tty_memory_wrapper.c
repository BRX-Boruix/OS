// TTY内存管理包装器
// 使用Rust内存管理器实现

#include "kernel/tty.h"
#include "rust/rust_memory.h"

// TTY内存管理函数实现（使用Rust内存管理器）
void* tty_kmalloc(size_t size) {
    return rust_kmalloc(size);
}

void tty_kfree(void* ptr) {
    rust_kfree(ptr);
}

void tty_memory_stats(size_t *total, size_t *used, size_t *free, size_t *peak) {
    rust_memory_stats_t stats = {0};
    if (rust_memory_stats(&stats) == 0) {
        if (total) *total = stats.heap.total_size;
        if (used) *used = stats.heap.allocated;
        if (free) *free = stats.heap.free;
        if (peak) *peak = stats.heap.peak_usage;
    }
}

void* tty_kmalloc_large(size_t size) {
    return rust_kmalloc(size);
}

void tty_kfree_large(void* ptr) {
    rust_kfree(ptr);
}

void* tty_map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    int ret = rust_map_page(virtual_addr, physical_addr, flags);
    return ret == 0 ? (void*)virtual_addr : NULL;
}

uint64_t tty_get_physical_addr(uint64_t virtual_addr) {
    return rust_virt_to_phys(virtual_addr);
}
