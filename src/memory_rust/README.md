# Boruix OS Rust内存管理器

x86_64架构的完整内存管理系统，使用Rust语言实现，提供高性能、内存安全的内存管理功能。

## 功能特性

### 核心功能
- **物理内存管理**: 页面分配器，支持单页和连续页面分配
- **虚拟内存管理**: 4级页表管理，支持页面映射和取消映射
- **内核堆分配器**: 动态内存分配，支持kmalloc/kfree接口
- **内存统计**: 详细的内存使用统计和调试信息

### 架构支持
- **x86_64**: 完整支持4级页表（PML4/PDP/PD/PT）
- **页面大小**: 4KB标准页面
- **地址空间**: 支持内核和用户地址空间分离

### 安全特性
- **内存安全**: Rust语言提供的内存安全保证
- **类型安全**: 强类型系统防止内存错误
- **无运行时**: 零成本抽象，无垃圾回收器

## 项目结构

```
src/
├── lib.rs          # 主库文件和全局内存管理器
├── arch.rs         # x86_64架构相关定义和常量
├── physical.rs     # 物理内存分配器
├── paging.rs       # 虚拟内存管理和页表操作
├── heap.rs         # 内核堆分配器
├── stats.rs        # 内存统计和调试功能
├── allocator.rs    # 分配器接口定义
└── ffi.rs          # C语言FFI接口

include/
└── rust_memory.h   # C语言头文件

Cargo.toml          # Rust项目配置
Makefile           # 构建脚本
README.md          # 项目文档
```

## 编译和构建

### 前置要求
- Rust 1.70+ (nightly)
- Cargo
- GCC (用于C包装器)
- GNU Make

### 构建命令

```bash
# 发布模式构建（推荐）
make release

# 调试模式构建
make debug

# 检查代码
make check

# 运行测试
make test

# 代码格式化
make fmt

# 静态分析
make clippy

# 生成文档
make doc

# 安装到内核目录
make install

# 清理构建文件
make clean
```

## 使用方法

### C语言接口

```c
#include "rust/rust_memory.h"

// 初始化内存管理器
rust_memory_region_t regions[] = {
    {0x100000, 0x7F00000, RUST_MEMORY_TYPE_AVAILABLE}, // 1MB-128MB
};
rust_memory_init(regions, 1);

// 分配内存
void* ptr = rust_kmalloc(1024);
if (ptr) {
    // 使用内存
    rust_kfree(ptr);
}

// 分配物理页面
uint64_t page = rust_alloc_page();
if (page) {
    rust_free_page(page);
}

// 页面映射
rust_map_page(0xFFFF800000000000, 0x100000, RUST_PAGE_KERNEL_RW);

// 获取统计信息
rust_memory_summary_t summary;
if (rust_memory_summary(&summary) == 0) {
    printf("Total: %lu MB, Used: %lu MB\n", 
           summary.total_physical_mb, summary.used_physical_mb);
}
```

### Rust接口

```rust
use boruix_memory::*;

// 创建内存管理器
let mut manager = MemoryManager::new();
manager.init(&memory_regions)?;

// 使用分配器接口
let ptr = kmalloc(1024)?;
kfree(ptr)?;

// 页面操作
let frame = alloc_page()?;
free_page(frame)?;

// 虚拟内存映射
map_page(virt_addr, phys_addr, PAGE_PRESENT | PAGE_WRITABLE)?;
```

## 内存布局

### 虚拟地址空间布局

```
0x0000000000000000 - 0x00007FFFFFFFFFFF  用户空间 (128TB)
0x0000800000000000 - 0xFFFF7FFFFFFFFFFF  未使用
0xFFFF800000000000 - 0xFFFFFFFF7FFFFFFF  内核直接映射
0xFFFFFFFF80000000 - 0xFFFFFFFF8FFFFFFF  内核代码段
0xFFFFFFFF90000000 - 0xFFFFFFFFA0000000  内核堆 (256MB)
0xFFFFFFFFA0000000 - 0xFFFFFFFFFFFFFFFF  内核栈和其他
```

### 物理内存管理

- **页面大小**: 4KB (4096字节)
- **分配单位**: 单页或连续页面
- **空闲列表**: 链表管理空闲页面
- **内存区域**: 支持多个不连续内存区域

## 性能特性

### 分配器性能
- **时间复杂度**: O(1) 平均分配时间
- **空间开销**: 最小化元数据开销
- **碎片化**: 智能合并算法减少碎片

### 页表管理
- **延迟分配**: 按需创建页表
- **TLB管理**: 自动TLB刷新
- **写时复制**: 支持COW页面

## 调试和诊断

### 内存统计
```c
rust_memory_stats_t stats;
rust_memory_stats(&stats);

printf("物理内存: %lu/%lu MB (%.1f%%)\n",
       stats.physical.allocated_memory / (1024*1024),
       stats.physical.total_memory / (1024*1024),
       stats.physical.usage_percent);

printf("堆内存: %lu/%lu KB\n",
       stats.heap.allocated / 1024,
       stats.heap.total_size / 1024);
```

### 内存检查
```c
// 检查内存完整性
int result = rust_memory_check();
switch (result) {
    case 0: printf("内存正常\n"); break;
    case 1: printf("发现内存泄漏\n"); break;
    case 2: printf("发现内存损坏\n"); break;
}

// 调试输出
rust_memory_debug_print();
```

### 压力测试
```c
// 内存压力测试
int result = rust_memory_stress_test(1000, 4096);
if (result == 0) {
    printf("压力测试通过\n");
}

// 性能基准测试
uint64_t time_us = rust_memory_benchmark(1000, 1024);
printf("1000次1KB分配耗时: %lu 微秒\n", time_us);
```

## 错误处理

### 常见错误
- **内存不足**: 物理内存或虚拟地址空间耗尽
- **无效地址**: 访问未映射的虚拟地址
- **权限错误**: 访问权限不足的页面
- **对齐错误**: 地址或大小未正确对齐

### 错误代码
```c
#define RUST_MEMORY_OK              0
#define RUST_MEMORY_ERROR_NOMEM    -1
#define RUST_MEMORY_ERROR_INVALID  -2
#define RUST_MEMORY_ERROR_PERM     -3
#define RUST_MEMORY_ERROR_ALIGN    -4
```

## 配置选项

### 编译时配置
```toml
[features]
default = ["paging", "heap", "stats", "debug"]
paging = []     # 页表管理
heap = []       # 堆分配器
stats = []      # 统计功能
debug = []      # 调试功能
```

### 运行时配置
```c
// 设置调试级别
rust_memory_set_debug_level(2); // 0=关闭, 1=基本, 2=详细, 3=完整
```

## 集成指南

### 与现有内核集成

1. **编译库文件**
   ```bash
   make release
   make install
   ```

2. **包含头文件**
   ```c
   #include "rust/rust_memory.h"
   ```

3. **链接库文件**
   ```makefile
   LIBS += -L./lib -lrust_memory
   ```

4. **初始化调用**
   ```c
   // 在内核初始化阶段调用
   rust_memory_init(memory_regions, region_count);
   ```

### 替换现有分配器

```c
// 替换kmalloc/kfree
#define kmalloc(size) rust_kmalloc(size)
#define kfree(ptr) rust_kfree(ptr)

// 替换页面分配
#define alloc_page() rust_alloc_page()
#define free_page(addr) rust_free_page(addr)
```

## 许可证

本项目采用与Boruix OS相同的许可证。

## 贡献

欢迎提交问题报告和改进建议。请确保：

1. 代码符合Rust标准格式 (`cargo fmt`)
2. 通过所有测试 (`cargo test`)
3. 通过静态分析 (`cargo clippy`)
4. 添加适当的文档和注释

## 版本历史

- **v1.0.0**: 初始版本，基本内存管理功能
- 支持x86_64架构
- 物理内存分配器
- 虚拟内存管理
- 内核堆分配器
- C语言接口
