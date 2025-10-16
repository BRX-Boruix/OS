// Boruix OS x86_64内存管理实现
// 64位页表管理和内存分配

#include "kernel/memory.h"
#include "kernel/kernel.h"
#include "drivers/display.h"

#ifdef __x86_64__

// 全局页表指针 - 使用引导代码设置的页表
extern uint64_t pml4_table[];
extern uint64_t pdp_table[];
extern uint64_t pd_table[];

// 内部页表指针
static uint64_t* kernel_pml4 = NULL;
static uint64_t* kernel_pdp = NULL;
static uint64_t* kernel_pd = NULL;

// 简单的内存分配器状态
static uint64_t next_free_page = 0x100000;  // 1MB开始
static uint64_t total_memory = 0;
static memory_region_t* memory_regions = NULL;

// 内核堆管理
typedef struct heap_block {
    size_t size;
    bool is_free;
    struct heap_block* next;
} heap_block_t;

static heap_block_t* heap_start = NULL;
static uint64_t heap_current = KERNEL_HEAP_START;

// 初始化x86_64内存管理
void memory_init_x86_64(uint64_t multiboot_info) {
    print_string("Initializing x86_64 memory management...\n");
    
    // 使用引导代码已设置的页表
    kernel_pml4 = pml4_table;
    kernel_pdp = pdp_table;
    kernel_pd = pd_table;
    
    // 简单的内存检测（这里使用固定值，实际应解析multiboot信息）
    total_memory = 128 * 1024 * 1024;  // 假设128MB内存
    next_free_page = 0x400000;  // 4MB开始分配
    
    print_string("- Total memory: 128MB\n");
    print_string("- Using existing page tables\n");
    print_string("- Page allocator initialized\n");
    
    // 初始化内核堆 - 使用更安全的地址
    heap_current = 0x800000;  // 8MB处开始堆，避免与页表冲突
    heap_start = (heap_block_t*)heap_current;
    heap_start->size = 0x800000;  // 8MB堆空间
    heap_start->is_free = true;
    heap_start->next = NULL;
    
    print_string("- Kernel heap initialized (8MB)\n");
    print_string("x86_64 memory management ready!\n");
}

// 分配物理页面
static uint64_t alloc_physical_page() {
    if (next_free_page >= total_memory) {
        return 0;  // 内存不足
    }
    
    uint64_t page = next_free_page;
    next_free_page += PAGE_SIZE;
    return page;
}

// 获取或创建页表项
static uint64_t* get_or_create_page_table(uint64_t* parent_table, int index) {
    if (!(parent_table[index] & PAGE_PRESENT)) {
        // 分配新的页表
        uint64_t new_table_phys = alloc_physical_page();
        if (new_table_phys == 0) {
            return NULL;  // 内存不足
        }
        
        // 清零新页表
        uint64_t* new_table = (uint64_t*)new_table_phys;
        for (int i = 0; i < 512; i++) {
            new_table[i] = 0;
        }
        
        // 设置父表项
        parent_table[index] = new_table_phys | PAGE_PRESENT | PAGE_WRITABLE;
    }
    
    return (uint64_t*)(parent_table[index] & PAGE_MASK);
}

// 映射虚拟页面到物理页面
void* map_page_x86_64(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    // 计算页表索引
    int pml4_idx = PML4_INDEX(virtual_addr);
    int pdp_idx = PDP_INDEX(virtual_addr);
    int pd_idx = PD_INDEX(virtual_addr);
    int pt_idx = PT_INDEX(virtual_addr);
    
    // 获取或创建PDP表
    uint64_t* pdp = get_or_create_page_table(kernel_pml4, pml4_idx);
    if (!pdp) return NULL;
    
    // 获取或创建PD表
    uint64_t* pd = get_or_create_page_table(pdp, pdp_idx);
    if (!pd) return NULL;
    
    // 获取或创建PT表
    uint64_t* pt = get_or_create_page_table(pd, pd_idx);
    if (!pt) return NULL;
    
    // 设置页表项
    pt[pt_idx] = (physical_addr & PAGE_MASK) | flags;
    
    // 刷新TLB
    __asm__ volatile ("invlpg (%0)" : : "r" (virtual_addr) : "memory");
    
    return (void*)virtual_addr;
}

// 取消页面映射
void unmap_page_x86_64(uint64_t virtual_addr) {
    int pml4_idx = PML4_INDEX(virtual_addr);
    int pdp_idx = PDP_INDEX(virtual_addr);
    int pd_idx = PD_INDEX(virtual_addr);
    int pt_idx = PT_INDEX(virtual_addr);
    
    // 检查页表是否存在
    if (!(kernel_pml4[pml4_idx] & PAGE_PRESENT)) return;
    
    uint64_t* pdp = (uint64_t*)(kernel_pml4[pml4_idx] & PAGE_MASK);
    if (!(pdp[pdp_idx] & PAGE_PRESENT)) return;
    
    uint64_t* pd = (uint64_t*)(pdp[pdp_idx] & PAGE_MASK);
    if (!(pd[pd_idx] & PAGE_PRESENT)) return;
    
    uint64_t* pt = (uint64_t*)(pd[pd_idx] & PAGE_MASK);
    
    // 清除页表项
    pt[pt_idx] = 0;
    
    // 刷新TLB
    __asm__ volatile ("invlpg (%0)" : : "r" (virtual_addr) : "memory");
}

// 获取虚拟地址对应的物理地址
uint64_t get_physical_addr_x86_64(uint64_t virtual_addr) {
    int pml4_idx = PML4_INDEX(virtual_addr);
    int pdp_idx = PDP_INDEX(virtual_addr);
    int pd_idx = PD_INDEX(virtual_addr);
    int pt_idx = PT_INDEX(virtual_addr);
    
    if (!(kernel_pml4[pml4_idx] & PAGE_PRESENT)) return 0;
    
    uint64_t* pdp = (uint64_t*)(kernel_pml4[pml4_idx] & PAGE_MASK);
    if (!(pdp[pdp_idx] & PAGE_PRESENT)) return 0;
    
    uint64_t* pd = (uint64_t*)(pdp[pdp_idx] & PAGE_MASK);
    if (!(pd[pd_idx] & PAGE_PRESENT)) return 0;
    
    uint64_t* pt = (uint64_t*)(pd[pd_idx] & PAGE_MASK);
    if (!(pt[pt_idx] & PAGE_PRESENT)) return 0;
    
    return (pt[pt_idx] & PAGE_MASK) | (virtual_addr & 0xFFF);
}

// 内核内存分配
void* kmalloc_x86_64(size_t size) {
    if (size == 0) return NULL;
    
    // 对齐到8字节
    size = (size + 7) & ~7;
    
    // 查找合适的空闲块
    heap_block_t* current = heap_start;
    while (current) {
        if (current->is_free && current->size >= size + sizeof(heap_block_t)) {
            // 找到合适的块
            if (current->size > size + sizeof(heap_block_t) + 32) {
                // 分割块
                heap_block_t* new_block = (heap_block_t*)((uint8_t*)current + sizeof(heap_block_t) + size);
                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->is_free = true;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->is_free = false;
            return (uint8_t*)current + sizeof(heap_block_t);
        }
        current = current->next;
    }
    
    return NULL;  // 内存不足
}

// 释放内核内存
void kfree_x86_64(void* ptr) {
    if (!ptr) return;
    
    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    block->is_free = true;
    
    // 简单的合并相邻空闲块
    heap_block_t* current = heap_start;
    while (current && current->next) {
        if (current->is_free && current->next->is_free) {
            current->size += current->next->size + sizeof(heap_block_t);
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

#endif // __x86_64__
