/**
 * Boruix OS Rust内存管理器 C接口头文件
 * x86_64架构内存管理系统的C语言接口
 */

#ifndef RUST_MEMORY_H
#define RUST_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 内存区域类型
typedef enum {
    RUST_MEMORY_TYPE_AVAILABLE = 1,
    RUST_MEMORY_TYPE_RESERVED = 2,
    RUST_MEMORY_TYPE_ACPI_RECLAIMABLE = 3,
    RUST_MEMORY_TYPE_ACPI_NVS = 4,
    RUST_MEMORY_TYPE_BAD = 5
} rust_memory_type_t;

// C兼容的内存区域结构
typedef struct {
    uint64_t base_addr;
    uint64_t length;
    uint32_t memory_type;
} rust_memory_region_t;

// 物理内存统计
typedef struct {
    uint64_t total_memory;
    uint64_t available_memory;
    uint64_t allocated_memory;
    uint64_t reserved_memory;
    size_t total_pages;
    size_t allocated_pages;
    size_t free_pages;
    uint32_t usage_percent;
} rust_physical_memory_stats_t;

// 虚拟内存统计
typedef struct {
    uint64_t kernel_virtual_size;
    uint64_t user_virtual_size;
    size_t mapped_pages;
    size_t page_table_memory;
} rust_virtual_memory_stats_t;

// 堆统计信息
typedef struct {
    size_t total_size;
    size_t allocated;
    size_t free;
    size_t peak_usage;
    size_t total_allocated;
    size_t total_freed;
} rust_heap_stats_t;

// 页表统计
typedef struct {
    size_t pml4_tables;
    size_t pdp_tables;
    size_t pd_tables;
    size_t pt_tables;
    size_t total_entries;
    size_t valid_entries;
} rust_page_table_stats_t;

// 内存统计信息
typedef struct {
    rust_physical_memory_stats_t physical;
    rust_virtual_memory_stats_t virtual_memory;
    rust_heap_stats_t heap;
    rust_page_table_stats_t page_tables;
} rust_memory_stats_t;

// 内存使用摘要
typedef struct {
    uint64_t total_physical_mb;
    uint64_t used_physical_mb;
    uint64_t free_physical_mb;
    size_t heap_used_kb;
    size_t heap_free_kb;
    size_t page_tables_count;
    uint32_t usage_percent;
} rust_memory_summary_t;

// 内存报告
typedef struct {
    rust_memory_summary_t summary;
    size_t leak_count;
    uint32_t fragmentation_percent;
    size_t region_count;
    size_t allocation_count;
} rust_memory_report_t;

// 功能特性标志
#define RUST_MEMORY_FEATURE_PAGING  (1 << 0)
#define RUST_MEMORY_FEATURE_HEAP    (1 << 1)
#define RUST_MEMORY_FEATURE_STATS   (1 << 2)
#define RUST_MEMORY_FEATURE_DEBUG   (1 << 3)

// 页面标志
#define RUST_PAGE_PRESENT           0x001
#define RUST_PAGE_WRITABLE          0x002
#define RUST_PAGE_USER              0x004
#define RUST_PAGE_WRITE_THROUGH     0x008
#define RUST_PAGE_CACHE_DISABLE     0x010
#define RUST_PAGE_ACCESSED          0x020
#define RUST_PAGE_DIRTY             0x040
#define RUST_PAGE_SIZE_FLAG         0x080
#define RUST_PAGE_GLOBAL            0x100
#define RUST_PAGE_NO_EXECUTE        0x8000000000000000ULL

/**
 * 初始化Rust内存管理器
 * 
 * @param memory_regions 内存区域数组指针
 * @param region_count 内存区域数量
 * @return 0表示成功，-1表示失败
 */
int rust_memory_init(const rust_memory_region_t* memory_regions, size_t region_count);

/**
 * 分配内存（kmalloc的C接口）
 * 
 * @param size 要分配的字节数
 * @return 非空指针表示成功，空指针表示失败
 */
void* rust_kmalloc(size_t size);

/**
 * 释放内存（kfree的C接口）
 * 
 * @param ptr 要释放的内存指针
 */
void rust_kfree(void* ptr);

/**
 * 分配物理页面
 * 
 * @return 物理页面地址（页面对齐），0表示失败
 */
uint64_t rust_alloc_page(void);

/**
 * 释放物理页面
 * 
 * @param page_addr 物理页面地址
 */
void rust_free_page(uint64_t page_addr);

/**
 * 映射虚拟页面到物理页面
 * 
 * @param virtual_addr 虚拟地址
 * @param physical_addr 物理地址
 * @param flags 页面标志
 * @return 0表示成功，-1表示失败
 */
int rust_map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);

/**
 * 取消页面映射
 * 
 * @param virtual_addr 虚拟地址
 * @return 物理地址表示成功，0表示失败
 */
uint64_t rust_unmap_page(uint64_t virtual_addr);

/**
 * 虚拟地址转物理地址
 * 
 * @param virtual_addr 虚拟地址
 * @return 物理地址表示成功，0表示转换失败
 */
uint64_t rust_virt_to_phys(uint64_t virtual_addr);

/**
 * 获取内存统计信息
 * 
 * @param stats 输出统计信息的指针
 * @return 0表示成功，-1表示失败
 */
int rust_memory_stats(rust_memory_stats_t* stats);

/**
 * 获取内存使用摘要
 * 
 * @param summary 输出摘要信息的指针
 * @return 0表示成功，-1表示失败
 */
int rust_memory_summary(rust_memory_summary_t* summary);

/**
 * 生成内存报告
 * 
 * @param report 输出报告的指针
 * @return 0表示成功，-1表示失败
 */
int rust_memory_report(rust_memory_report_t* report);

/**
 * 检查内存完整性
 * 
 * @return 0表示正常，1表示内存泄漏，2表示内存损坏，-1表示检查失败
 */
int rust_memory_check(void);

/**
 * 打印内存使用信息（调试用）
 */
void rust_memory_debug_print(void);

/**
 * 设置内存调试级别
 * 
 * @param level 调试级别 (0=关闭, 1=基本, 2=详细, 3=完整)
 */
void rust_memory_set_debug_level(uint32_t level);

/**
 * 内存压力测试
 * 
 * @param iterations 测试迭代次数
 * @param max_alloc_size 最大分配大小
 * @return 0表示测试通过，-1表示测试失败
 */
int rust_memory_stress_test(uint32_t iterations, size_t max_alloc_size);

/**
 * 内存基准测试
 * 
 * @param alloc_count 分配次数
 * @param alloc_size 分配大小
 * @return 测试耗时（微秒），0表示测试失败
 */
uint64_t rust_memory_benchmark(uint32_t alloc_count, size_t alloc_size);

/**
 * 获取内存管理器版本信息
 * 
 * @return 版本字符串指针
 */
const char* rust_memory_version(void);

/**
 * 获取支持的功能特性
 * 
 * @return 功能特性标志位掩码
 */
uint32_t rust_memory_features(void);

// 便利宏定义
#define RUST_KMALLOC(size) rust_kmalloc(size)
#define RUST_KFREE(ptr) rust_kfree(ptr)
#define RUST_ALLOC_PAGE() rust_alloc_page()
#define RUST_FREE_PAGE(addr) rust_free_page(addr)
#define RUST_MAP_PAGE(virt, phys, flags) rust_map_page(virt, phys, flags)
#define RUST_UNMAP_PAGE(virt) rust_unmap_page(virt)
#define RUST_VIRT_TO_PHYS(virt) rust_virt_to_phys(virt)

// 常用页面标志组合
#define RUST_PAGE_KERNEL_RW     (RUST_PAGE_PRESENT | RUST_PAGE_WRITABLE)
#define RUST_PAGE_KERNEL_RO     (RUST_PAGE_PRESENT)
#define RUST_PAGE_USER_RW       (RUST_PAGE_PRESENT | RUST_PAGE_WRITABLE | RUST_PAGE_USER)
#define RUST_PAGE_USER_RO       (RUST_PAGE_PRESENT | RUST_PAGE_USER)

#ifdef __cplusplus
}
#endif

#endif // RUST_MEMORY_H
