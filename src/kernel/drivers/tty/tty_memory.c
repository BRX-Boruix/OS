// Boruix OS TTY专用内存管理
// 完整的内存分配器，支持真正的内存管理

#include "kernel/memory.h"

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

// 初始化内存管理
void tty_memory_init(void) {
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
