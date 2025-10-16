// Boruix OS i386内存管理实现
// 32位页表管理和内存分配

#include "kernel/memory.h"
#include "kernel/kernel.h"
#include "drivers/display.h"

#ifndef __x86_64__

// 全局页表指针
static uint32_t* page_directory = (uint32_t*)0x1000;
static uint32_t next_free_page = 0x100000;  // 1MB开始
static uint32_t total_memory = 0;

// 简单的内核堆管理
typedef struct heap_block_32 {
    size_t size;
    bool is_free;
    struct heap_block_32* next;
} heap_block_32_t;

static heap_block_32_t* heap_start = NULL;
static uint32_t heap_current = KERNEL_HEAP_START_32;

// 初始化i386内存管理
void memory_init_i386(uint32_t multiboot_info) {
    print_string("Initializing i386 memory management...\n");
    
    // 简单的内存检测
    total_memory = 64 * 1024 * 1024;  // 假设64MB内存
    next_free_page = 0x400000;  // 4MB开始分配
    
    print_string("- Total memory: 64MB\n");
    print_string("- Page allocator initialized\n");
    
    // 初始化内核堆
    heap_start = (heap_block_32_t*)heap_current;
    heap_start->size = 0x800000;  // 8MB堆空间
    heap_start->is_free = true;
    heap_start->next = NULL;
    
    print_string("- Kernel heap initialized (8MB)\n");
    print_string("i386 memory management ready!\n");
}

// 分配物理页面
static uint32_t alloc_physical_page_32() {
    if (next_free_page >= total_memory) {
        return 0;  // 内存不足
    }
    
    uint32_t page = next_free_page;
    next_free_page += PAGE_SIZE;
    return page;
}

// 映射虚拟页面到物理页面
void* map_page_i386(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t pd_index = PD_INDEX_32(virtual_addr);
    uint32_t pt_index = PT_INDEX_32(virtual_addr);
    
    // 检查页表是否存在
    if (!(page_directory[pd_index] & PAGE_PRESENT)) {
        // 分配新的页表
        uint32_t new_table = alloc_physical_page_32();
        if (new_table == 0) return NULL;
        
        // 清零页表
        uint32_t* table = (uint32_t*)new_table;
        for (int i = 0; i < 1024; i++) {
            table[i] = 0;
        }
        
        page_directory[pd_index] = new_table | PAGE_PRESENT | PAGE_WRITABLE;
    }
    
    // 获取页表
    uint32_t* page_table = (uint32_t*)(page_directory[pd_index] & PAGE_MASK);
    
    // 设置页表项
    page_table[pt_index] = (physical_addr & PAGE_MASK) | flags;
    
    // 刷新TLB
    __asm__ volatile ("invlpg (%0)" : : "r" (virtual_addr) : "memory");
    
    return (void*)virtual_addr;
}

// 取消页面映射
void unmap_page_i386(uint32_t virtual_addr) {
    uint32_t pd_index = PD_INDEX_32(virtual_addr);
    uint32_t pt_index = PT_INDEX_32(virtual_addr);
    
    if (!(page_directory[pd_index] & PAGE_PRESENT)) return;
    
    uint32_t* page_table = (uint32_t*)(page_directory[pd_index] & PAGE_MASK);
    page_table[pt_index] = 0;
    
    // 刷新TLB
    __asm__ volatile ("invlpg (%0)" : : "r" (virtual_addr) : "memory");
}

// 获取虚拟地址对应的物理地址
uint32_t get_physical_addr_i386(uint32_t virtual_addr) {
    uint32_t pd_index = PD_INDEX_32(virtual_addr);
    uint32_t pt_index = PT_INDEX_32(virtual_addr);
    
    if (!(page_directory[pd_index] & PAGE_PRESENT)) return 0;
    
    uint32_t* page_table = (uint32_t*)(page_directory[pd_index] & PAGE_MASK);
    if (!(page_table[pt_index] & PAGE_PRESENT)) return 0;
    
    return (page_table[pt_index] & PAGE_MASK) | (virtual_addr & 0xFFF);
}

// 内核内存分配
void* kmalloc_i386(size_t size) {
    if (size == 0) return NULL;
    
    // 对齐到4字节
    size = (size + 3) & ~3;
    
    // 查找合适的空闲块
    heap_block_32_t* current = heap_start;
    while (current) {
        if (current->is_free && current->size >= size + sizeof(heap_block_32_t)) {
            // 找到合适的块
            if (current->size > size + sizeof(heap_block_32_t) + 16) {
                // 分割块
                heap_block_32_t* new_block = (heap_block_32_t*)((uint8_t*)current + sizeof(heap_block_32_t) + size);
                new_block->size = current->size - size - sizeof(heap_block_32_t);
                new_block->is_free = true;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->is_free = false;
            return (uint8_t*)current + sizeof(heap_block_32_t);
        }
        current = current->next;
    }
    
    return NULL;  // 内存不足
}

// 释放内核内存
void kfree_i386(void* ptr) {
    if (!ptr) return;
    
    heap_block_32_t* block = (heap_block_32_t*)((uint8_t*)ptr - sizeof(heap_block_32_t));
    block->is_free = true;
    
    // 简单的合并相邻空闲块
    heap_block_32_t* current = heap_start;
    while (current && current->next) {
        if (current->is_free && current->next->is_free) {
            current->size += current->next->size + sizeof(heap_block_32_t);
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

#endif // !__x86_64__
