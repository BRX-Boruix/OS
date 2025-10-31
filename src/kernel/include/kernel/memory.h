// Boruix OS 内存管理头文件
// 使用Rust内存管理器实现

#ifndef BORUIX_MEMORY_H
#define BORUIX_MEMORY_H

#include "kernel/types.h"
#include "rust/rust_memory.h"

// 页面大小常量（保持兼容性）
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define PAGE_MASK (~(PAGE_SIZE - 1))

// 页面标志（映射到Rust定义）
#define PAGE_PRESENT            RUST_PAGE_PRESENT
#define PAGE_WRITABLE           RUST_PAGE_WRITABLE
#define PAGE_USER               RUST_PAGE_USER
#define PAGE_WRITETHROUGH       RUST_PAGE_WRITE_THROUGH
#define PAGE_CACHE_DISABLE      RUST_PAGE_CACHE_DISABLE
#define PAGE_ACCESSED           RUST_PAGE_ACCESSED
#define PAGE_DIRTY              RUST_PAGE_DIRTY
#define PAGE_SIZE_FLAG          RUST_PAGE_SIZE_FLAG
#define PAGE_GLOBAL             RUST_PAGE_GLOBAL
#define PAGE_NO_EXECUTE         RUST_PAGE_NO_EXECUTE

// 内存区域类型（映射到Rust定义）
typedef enum {
    MEMORY_TYPE_AVAILABLE = RUST_MEMORY_TYPE_AVAILABLE,
    MEMORY_TYPE_RESERVED = RUST_MEMORY_TYPE_RESERVED,
    MEMORY_TYPE_ACPI_RECLAIMABLE = RUST_MEMORY_TYPE_ACPI_RECLAIMABLE,
    MEMORY_TYPE_ACPI_NVS = RUST_MEMORY_TYPE_ACPI_NVS,
    MEMORY_TYPE_BAD = RUST_MEMORY_TYPE_BAD
} memory_type_t;

// 内存区域描述符（映射到Rust定义）
typedef rust_memory_region_t memory_region_t;

#ifdef __x86_64__

// 页表索引宏（保持兼容性）
#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDP_INDEX(addr)  (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((addr) >> 12) & 0x1FF)

// 虚拟地址空间布局（保持兼容性）
#define KERNEL_VIRTUAL_BASE   0xFFFFFFFF80000000ULL
#define USER_VIRTUAL_BASE     0x0000000000400000ULL
#define USER_VIRTUAL_END      0x00007FFFFFFFFFFFULL
#define KERNEL_HEAP_START     0xFFFFFFFF90000000ULL
#define KERNEL_HEAP_END       0xFFFFFFFFA0000000ULL

// 内存管理函数（使用Rust内存管理器）
static inline int memory_init_x86_64(uint64_t multiboot_info) {
    (void)multiboot_info;
    // 创建内存区域映射
    // 使用懒加载伙伴分配器：支持大内存，快速初始化
    rust_memory_region_t regions[] = {
        {0x1000000, 0x3F000000, RUST_MEMORY_TYPE_AVAILABLE}, // 16MB-1008MB (992MB可用)
    };
    return rust_memory_init(regions, 1);
}

static inline void* kmalloc_x86_64(size_t size) {
    return rust_kmalloc(size);
}

static inline void kfree_x86_64(void* ptr) {
    rust_kfree(ptr);
}

static inline void* map_page_x86_64(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    (void)physical_addr;
    (void)flags;
    // 暂时不支持页面映射
    return (void*)virtual_addr;
}

static inline void unmap_page_x86_64(uint64_t virtual_addr) {
    (void)virtual_addr;
    // 暂时不支持页面取消映射
}

static inline uint64_t get_physical_addr_x86_64(uint64_t virtual_addr) {
    // 暂时假设身份映射
    return virtual_addr;
}

#else

// 32位支持（已废弃，保留定义）
#define PD_INDEX_32(addr)  (((addr) >> 22) & 0x3FF)
#define PT_INDEX_32(addr)  (((addr) >> 12) & 0x3FF)

#define KERNEL_VIRTUAL_BASE_32   0xC0000000
#define USER_VIRTUAL_BASE_32     0x00400000
#define USER_VIRTUAL_END_32      0xBFFFFFFF
#define KERNEL_HEAP_START_32     0xD0000000
#define KERNEL_HEAP_END_32       0xE0000000

#endif

// 通用内存管理接口（映射到Rust内存管理器）
#define memory_init(info) memory_init_x86_64(info)
#define kmalloc(size) rust_kmalloc(size)
#define kfree(ptr) rust_kfree(ptr)
#define map_page(vaddr, paddr, flags) map_page_x86_64(vaddr, paddr, flags)
#define unmap_page(vaddr) unmap_page_x86_64(vaddr)
#define get_physical_addr(vaddr) get_physical_addr_x86_64(vaddr)

// 通用函数（这些保持C实现，因为它们是基础函数）
void* memset(void* dest, int value, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
int memcmp(const void* ptr1, const void* ptr2, size_t count);

// 页面分配函数（使用Rust分配器）
static inline uint64_t alloc_page(void) {
    return rust_alloc_page();
}

static inline void free_page(uint64_t page_addr) {
    rust_free_page(page_addr);
}


#endif // BORUIX_MEMORY_H