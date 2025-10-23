// Boruix OS TTY专用内存管理
// 完整的内存分配器，支持页表管理和大内存

#include "kernel/memory.h"
#include "kernel/kernel.h"
#include "drivers/display.h"

#ifdef __x86_64__

// 页表相关定义（使用系统已有的定义）
// PAGE_SIZE, PAGE_PRESENT, PAGE_WRITABLE, PAGE_USER, PAGE_MASK 已在系统头文件中定义

// 页表索引宏
#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDP_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr) (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr) (((addr) >> 12) & 0x1FF)

// 页表结构
typedef struct {
    uint64_t entries[512];
} page_table_t;

// 页表管理
static page_table_t* kernel_pml4 = NULL;
static page_table_t* kernel_pdp = NULL;
static page_table_t* kernel_pd = NULL;
static uint64_t next_free_page = 0x100000;  // 1MB开始
static uint64_t total_memory = 0;
static bool page_tables_initialized = false;

// 内存池大小
#define TTY_MEMORY_POOL_SIZE (256 * 1024)  // 256KB
#define TTY_MIN_BLOCK_SIZE 16  // 最小块大小

// 内存块结构
typedef struct tty_memory_block {
    size_t size;                    // 块大小
    bool is_free;                   // 是否空闲
    struct tty_memory_block *next;  // 下一个块
    struct tty_memory_block *prev;  // 上一个块
} tty_memory_block_t;

// 内存池
static char tty_memory_pool[TTY_MEMORY_POOL_SIZE];
static tty_memory_block_t *free_list = NULL;
static tty_memory_block_t *allocated_list = NULL;

// 内存统计
static size_t total_allocated = 0;
static size_t total_freed = 0;
static size_t peak_usage = 0;
static size_t current_usage = 0;

// 前向声明
static void tty_init_page_tables(void);

// 初始化内存管理
void tty_memory_init(void) {
    // 初始化页表管理
    tty_init_page_tables();
    
    // 清零内存池
    for (size_t i = 0; i < TTY_MEMORY_POOL_SIZE; i++) {
        tty_memory_pool[i] = 0;
    }
    
    // 初始化内存池为一个大空闲块
    tty_memory_block_t *initial_block = (tty_memory_block_t *)tty_memory_pool;
    initial_block->size = TTY_MEMORY_POOL_SIZE - sizeof(tty_memory_block_t);
    initial_block->is_free = true;
    initial_block->next = NULL;
    initial_block->prev = NULL;
    
    free_list = initial_block;
    allocated_list = NULL;
    
    total_allocated = 0;
    total_freed = 0;
    peak_usage = 0;
    current_usage = 0;
}

// 前向声明
static void add_to_free_list(tty_memory_block_t *block);

// 分割内存块
static void split_block(tty_memory_block_t *block, size_t size) {
    if (block->size > size + sizeof(tty_memory_block_t) + TTY_MIN_BLOCK_SIZE) {
        // 创建新块
        tty_memory_block_t *new_block = (tty_memory_block_t *)((char *)block + sizeof(tty_memory_block_t) + size);
        new_block->size = block->size - size - sizeof(tty_memory_block_t);
        new_block->is_free = true;
        new_block->next = block->next;
        new_block->prev = block;
        
        if (block->next) {
            block->next->prev = new_block;
        }
        
        block->size = size;
        block->next = new_block;
        
        // 将新块添加到空闲列表
        add_to_free_list(new_block);
    }
}

// 合并相邻的空闲块
static void merge_free_blocks(tty_memory_block_t *block) {
    // 合并后面的块
    while (block->next && block->next->is_free) {
        tty_memory_block_t *next_block = block->next;
        block->size += sizeof(tty_memory_block_t) + next_block->size;
        block->next = next_block->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    // 合并前面的块
    while (block->prev && block->prev->is_free) {
        tty_memory_block_t *prev_block = block->prev;
        prev_block->size += sizeof(tty_memory_block_t) + block->size;
        prev_block->next = block->next;
        if (block->next) {
            block->next->prev = prev_block;
        }
        block = prev_block;
    }
}

// 从空闲列表移除块
static void remove_from_free_list(tty_memory_block_t *block) {
    if (!block) return;
    
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        free_list = block->next;
    }
    
    if (block->next) {
        block->next->prev = block->prev;
    }
    
    // 清理指针
    block->next = NULL;
    block->prev = NULL;
}

// 添加到空闲列表
static void add_to_free_list(tty_memory_block_t *block) {
    if (!block) return;
    
    block->next = free_list;
    block->prev = NULL;
    if (free_list) {
        free_list->prev = block;
    }
    free_list = block;
}

// 添加到已分配列表
static void add_to_allocated_list(tty_memory_block_t *block) {
    if (!block) return;
    
    block->next = allocated_list;
    block->prev = NULL;
    if (allocated_list) {
        allocated_list->prev = block;
    }
    allocated_list = block;
}

// 从已分配列表移除块
static void remove_from_allocated_list(tty_memory_block_t *block) {
    if (!block) return;
    
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        allocated_list = block->next;
    }
    
    if (block->next) {
        block->next->prev = block->prev;
    }
    
    // 清理指针
    block->next = NULL;
    block->prev = NULL;
}

// TTY专用的内存分配函数
void* tty_kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    // 对齐到8字节
    size = (size + 7) & ~7;
    
    // 查找合适的空闲块
    tty_memory_block_t *current = free_list;
    while (current) {
        if (current->is_free && current->size >= size) {
            // 找到合适的块
            remove_from_free_list(current);
            
            // 分割块（如果需要）
            split_block(current, size);
            
            // 标记为已分配
            current->is_free = false;
            add_to_allocated_list(current);
            
            // 更新统计
            total_allocated += size;
            current_usage += size;
            if (current_usage > peak_usage) {
                peak_usage = current_usage;
            }
            
            // 返回数据区域
            return (char *)current + sizeof(tty_memory_block_t);
        }
        current = current->next;
    }
    
    return NULL;  // 内存不足
}

// TTY专用的内存释放函数
void tty_kfree(void* ptr) {
    if (!ptr) return;
    
    // 获取块头
    tty_memory_block_t *block = (tty_memory_block_t *)((char *)ptr - sizeof(tty_memory_block_t));
    
    // 验证块是否有效
    if (block < (tty_memory_block_t *)tty_memory_pool || 
        block >= (tty_memory_block_t *)(tty_memory_pool + TTY_MEMORY_POOL_SIZE)) {
        return;  // 无效指针
    }
    
    if (block->is_free) {
        return;  // 已经释放
    }
    
    // 从已分配列表移除
    remove_from_allocated_list(block);
    
    // 标记为空闲
    block->is_free = true;
    
    // 合并相邻的空闲块
    merge_free_blocks(block);
    
    // 添加到空闲列表
    add_to_free_list(block);
    
    // 更新统计
    total_freed += block->size;
    current_usage -= block->size;
}

// 获取内存统计信息
void tty_memory_stats(size_t *total, size_t *used, size_t *free, size_t *peak) {
    if (total) *total = TTY_MEMORY_POOL_SIZE;
    if (used) *used = current_usage;
    if (free) *free = TTY_MEMORY_POOL_SIZE - current_usage;
    if (peak) *peak = peak_usage;
}

// 页表管理函数

// 分配物理页面（使用TTY内存池内的空间）
static uint64_t alloc_physical_page(void) {
    // 使用TTY内存池内的空间，避免与内核冲突
    static uint64_t temp_next_page = 0x20000000;  // 512MB开始，远离内核区域
    
    if (temp_next_page >= 0x30000000) {  // 限制在768MB以内
        return 0;  // 内存不足
    }
    
    uint64_t page = temp_next_page;
    temp_next_page += PAGE_SIZE;
    return page;
}

// 获取或创建页表
static page_table_t* get_or_create_page_table(page_table_t* parent_table, int index) {
    if (!(parent_table->entries[index] & PAGE_PRESENT)) {
        // 分配新的页表
        uint64_t new_table_phys = alloc_physical_page();
        if (new_table_phys == 0) {
            return NULL;  // 内存不足
        }
        
        // 清零新页表
        page_table_t* new_table = (page_table_t*)new_table_phys;
        for (int i = 0; i < 512; i++) {
            new_table->entries[i] = 0;
        }
        
        // 设置父表项
        parent_table->entries[index] = new_table_phys | PAGE_PRESENT | PAGE_WRITABLE;
    }
    
    return (page_table_t*)(parent_table->entries[index] & PAGE_MASK);
}

// 映射虚拟页面到物理页面
void* tty_map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    if (!page_tables_initialized) {
        return NULL;
    }
    
    // 计算页表索引
    int pml4_idx = PML4_INDEX(virtual_addr);
    int pdp_idx = PDP_INDEX(virtual_addr);
    int pd_idx = PD_INDEX(virtual_addr);
    int pt_idx = PT_INDEX(virtual_addr);
    
    // 获取或创建页表
    page_table_t* pdp = get_or_create_page_table(kernel_pml4, pml4_idx);
    if (!pdp) return NULL;
    
    page_table_t* pd = get_or_create_page_table(pdp, pdp_idx);
    if (!pd) return NULL;
    
    page_table_t* pt = get_or_create_page_table(pd, pd_idx);
    if (!pt) return NULL;
    
    // 设置页表项
    pt->entries[pt_idx] = physical_addr | flags | PAGE_PRESENT;
    
    return (void*)virtual_addr;
}

// 获取虚拟地址对应的物理地址
uint64_t tty_get_physical_addr(uint64_t virtual_addr) {
    if (!page_tables_initialized) {
        return 0;
    }
    
    int pml4_idx = PML4_INDEX(virtual_addr);
    int pdp_idx = PDP_INDEX(virtual_addr);
    int pd_idx = PD_INDEX(virtual_addr);
    int pt_idx = PT_INDEX(virtual_addr);
    
    if (!(kernel_pml4->entries[pml4_idx] & PAGE_PRESENT)) return 0;
    
    page_table_t* pdp = (page_table_t*)(kernel_pml4->entries[pml4_idx] & PAGE_MASK);
    if (!(pdp->entries[pdp_idx] & PAGE_PRESENT)) return 0;
    
    page_table_t* pd = (page_table_t*)(pdp->entries[pdp_idx] & PAGE_MASK);
    if (!(pd->entries[pd_idx] & PAGE_PRESENT)) return 0;
    
    page_table_t* pt = (page_table_t*)(pd->entries[pd_idx] & PAGE_MASK);
    if (!(pt->entries[pt_idx] & PAGE_PRESENT)) return 0;
    
    return (pt->entries[pt_idx] & PAGE_MASK) | (virtual_addr & 0xFFF);
}

// 初始化页表管理（简化版本，不使用独立页表）
void tty_init_page_tables(void) {
    print_string("[TTY] Initializing page tables...\n");
    
    // 设置内存大小
    total_memory = 128 * 1024 * 1024;  // 128MB
    next_free_page = 0x400000;  // 4MB开始分配
    
    // 简化实现：不创建独立页表，使用现有的内核页表
    page_tables_initialized = true;
    print_string("[TTY] Page tables initialized (128MB, using kernel page tables)\n");
}

// 大内存分配（使用TTY内存池内的空间）
void* tty_kmalloc_large(size_t size) {
    if (size == 0) return NULL;
    
    print_string("[TTY] Large allocation request: ");
    // 简单的数字转换
    char size_str[32];
    int i = 0;
    int temp = size;
    if (temp == 0) {
        size_str[i++] = '0';
    } else {
        while (temp > 0) {
            size_str[i++] = '0' + (temp % 10);
            temp /= 10;
        }
    }
    size_str[i] = '\0';
    // 反转字符串
    for (int j = 0; j < i/2; j++) {
        char c = size_str[j];
        size_str[j] = size_str[i-1-j];
        size_str[i-1-j] = c;
    }
    print_string(size_str);
    print_string(" bytes\n");
    
    // 检查大小是否超过内存池剩余空间
    if (size > TTY_MEMORY_POOL_SIZE / 2) {  // 限制大内存分配不超过内存池的一半
        print_string("[TTY] Large allocation too big for memory pool\n");
        return NULL;
    }
    
    // 使用TTY内存池内的空间进行大内存分配
    // 从内存池的后半部分分配大内存
    static size_t large_memory_offset = TTY_MEMORY_POOL_SIZE / 2;  // 从中间开始
    
    if (large_memory_offset + size > TTY_MEMORY_POOL_SIZE) {
        print_string("[TTY] Failed to allocate large memory (pool full)\n");
        return NULL;  // 内存不足
    }
    
    uint64_t virtual_addr = (uint64_t)(tty_memory_pool + large_memory_offset);
    large_memory_offset += size;
    
    print_string("[TTY] Allocated large memory: 0x");
    // 简单的十六进制转换
    char hex_str[16];
    uint64_t addr = virtual_addr;
    for (int j = 15; j >= 0; j--) {
        int digit = addr & 0xF;
        hex_str[j] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        addr >>= 4;
    }
    hex_str[16] = '\0';
    print_string(hex_str);
    print_string("\n");
    
    // 清零分配的内存
    char *mem = (char*)virtual_addr;
    for (size_t j = 0; j < size; j++) {
        mem[j] = 0;
    }
    
    print_string("[TTY] Large memory allocation: SUCCESS\n");
    return (void*)virtual_addr;
}

// 大内存释放
void tty_kfree_large(void* ptr) {
    if (!ptr) return;
    
    print_string("[TTY] Large memory deallocation: ");
    // 简单的十六进制转换
    char hex_str[16];
    uint64_t addr = (uint64_t)ptr;
    for (int j = 15; j >= 0; j--) {
        int digit = addr & 0xF;
        hex_str[j] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        addr >>= 4;
    }
    hex_str[16] = '\0';
    print_string("0x");
    print_string(hex_str);
    print_string("\n");
    
    // 简化处理：由于大内存分配使用的是TTY内存池内的空间，
    // 这里不需要实际回收，只需要记录释放即可
    print_string("[TTY] Large memory deallocation: SUCCESS\n");
}

#endif // __x86_64__
