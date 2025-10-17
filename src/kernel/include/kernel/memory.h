// Boruix OS 内存管理头文件
// 支持双架构的内存管理系统

#ifndef BORUIX_MEMORY_H
#define BORUIX_MEMORY_H

#include "kernel/types.h"

// 页面大小常量
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define PAGE_MASK (~(PAGE_SIZE - 1))

// 页面标志 (在arch/x86_64.h中定义)
#define PAGE_WRITETHROUGH 0x008
#define PAGE_CACHE_DISABLE 0x010
#define PAGE_ACCESSED   0x020
#define PAGE_DIRTY      0x040
#define PAGE_SIZE_FLAG  0x080  // 2MB页面
#define PAGE_GLOBAL     0x100
#define PAGE_NO_EXECUTE 0x8000000000000000ULL  // 仅x86_64

// 内存区域类型
typedef enum {
    MEMORY_TYPE_AVAILABLE = 1,
    MEMORY_TYPE_RESERVED = 2,
    MEMORY_TYPE_ACPI_RECLAIMABLE = 3,
    MEMORY_TYPE_ACPI_NVS = 4,
    MEMORY_TYPE_BAD = 5
} memory_type_t;

// 内存区域描述符
typedef struct memory_region {
    uint64_t base_addr;
    uint64_t length;
    memory_type_t type;
    struct memory_region* next;
} memory_region_t;

#ifdef __x86_64__

// x86_64页表结构
typedef struct {
    uint64_t present : 1;
    uint64_t writable : 1;
    uint64_t user : 1;
    uint64_t write_through : 1;
    uint64_t cache_disable : 1;
    uint64_t accessed : 1;
    uint64_t dirty : 1;
    uint64_t page_size : 1;
    uint64_t global : 1;
    uint64_t available : 3;
    uint64_t address : 40;
    uint64_t available2 : 11;
    uint64_t no_execute : 1;
} __attribute__((packed)) page_entry_64_t;

// 页表级别
#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDP_INDEX(addr)  (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((addr) >> 12) & 0x1FF)

// 虚拟地址空间布局
#define KERNEL_VIRTUAL_BASE   0xFFFFFFFF80000000ULL
#define USER_VIRTUAL_BASE     0x0000000000400000ULL
#define USER_VIRTUAL_END      0x00007FFFFFFFFFFFULL
#define KERNEL_HEAP_START     0xFFFFFFFF90000000ULL
#define KERNEL_HEAP_END       0xFFFFFFFFA0000000ULL

// 64位内存管理函数
void memory_init_x86_64(uint64_t multiboot_info);
void* kmalloc_x86_64(size_t size);
void kfree_x86_64(void* ptr);
void* map_page_x86_64(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);
void unmap_page_x86_64(uint64_t virtual_addr);
uint64_t get_physical_addr_x86_64(uint64_t virtual_addr);

#else

// i386页表结构
typedef struct {
    uint32_t present : 1;
    uint32_t writable : 1;
    uint32_t user : 1;
    uint32_t write_through : 1;
    uint32_t cache_disable : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t page_size : 1;
    uint32_t global : 1;
    uint32_t available : 3;
    uint32_t address : 20;
} __attribute__((packed)) page_entry_32_t;

// 页表索引
#define PD_INDEX_32(addr)  (((addr) >> 22) & 0x3FF)
#define PT_INDEX_32(addr)  (((addr) >> 12) & 0x3FF)

// 虚拟地址空间布局
#define KERNEL_VIRTUAL_BASE_32   0xC0000000
#define USER_VIRTUAL_BASE_32     0x00400000
#define USER_VIRTUAL_END_32      0xBFFFFFFF
#define KERNEL_HEAP_START_32     0xD0000000
#define KERNEL_HEAP_END_32       0xE0000000

// 32位内存管理函数
void memory_init_i386(uint32_t multiboot_info);
void* kmalloc_i386(size_t size);
void kfree_i386(void* ptr);
void* map_page_i386(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
void unmap_page_i386(uint32_t virtual_addr);
uint32_t get_physical_addr_i386(uint32_t virtual_addr);

#endif

// 通用内存管理接口
#ifdef __x86_64__
#define memory_init(info) memory_init_x86_64(info)
#define kmalloc(size) kmalloc_x86_64(size)
#define kfree(ptr) kfree_x86_64(ptr)
#define map_page(vaddr, paddr, flags) map_page_x86_64(vaddr, paddr, flags)
#define unmap_page(vaddr) unmap_page_x86_64(vaddr)
#define get_physical_addr(vaddr) get_physical_addr_x86_64(vaddr)
#else
#define memory_init(info) memory_init_i386(info)
#define kmalloc(size) kmalloc_i386(size)
#define kfree(ptr) kfree_i386(ptr)
#define map_page(vaddr, paddr, flags) map_page_i386(vaddr, paddr, flags)
#define unmap_page(vaddr) unmap_page_i386(vaddr)
#define get_physical_addr(vaddr) get_physical_addr_i386(vaddr)
#endif

// 通用函数
void* memset(void* dest, int value, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
int memcmp(const void* ptr1, const void* ptr2, size_t count);

#endif // BORUIX_MEMORY_H
