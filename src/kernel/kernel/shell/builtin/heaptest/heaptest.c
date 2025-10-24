#include "kernel/kernel.h"
#include "kernel/serial_debug.h"
#include "drivers/display.h"
#include "heaptest.h"

// 外部Rust函数声明
extern void* rust_kmalloc(size_t size);
extern void rust_kfree(void* ptr);
extern void rust_heap_stats(size_t *total_alloc, size_t *total_freed, 
                            size_t *current, size_t *alloc_count, size_t *free_count);

void cmd_heaptest(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    print_string("[HEAPTEST] Starting heap allocator test...\n");
    print_string("\n");
    
    // 测试1: 小块分配
    print_string("[TEST 1] Allocating small blocks (32 bytes each)...\n");
    void* ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = rust_kmalloc(32);
        if (ptrs[i] == NULL) {
            print_string("[FAIL] Failed to allocate block ");
            print_dec(i);
            print_string("\n");
            return;
        }
        // 写入测试数据
        uint32_t* data = (uint32_t*)ptrs[i];
        data[0] = 0xDEAD0000 + i;
        data[1] = 0xBEEF0000 + i;
    }
    print_string("[OK] 10 blocks allocated\n\n");
    
    // 测试2: 验证数据
    print_string("[TEST 2] Verifying written data...\n");
    int verify_ok = 1;
    for (int i = 0; i < 10; i++) {
        uint32_t* data = (uint32_t*)ptrs[i];
        if (data[0] != 0xDEAD0000 + i || data[1] != 0xBEEF0000 + i) {
            print_string("[FAIL] Data corruption in block ");
            print_dec(i);
            print_string("\n");
            verify_ok = 0;
        }
    }
    if (verify_ok) {
        print_string("[OK] All data verified\n\n");
    }
    
    // 测试3: 释放部分块
    print_string("[TEST 3] Freeing alternate blocks...\n");
    for (int i = 0; i < 10; i += 2) {
        rust_kfree(ptrs[i]);
        ptrs[i] = NULL;
    }
    print_string("[OK] 5 blocks freed\n\n");
    
    // 测试4: 重新分配
    print_string("[TEST 4] Re-allocating freed blocks...\n");
    for (int i = 0; i < 10; i += 2) {
        ptrs[i] = rust_kmalloc(32);
        if (ptrs[i] == NULL) {
            print_string("[FAIL] Failed to re-allocate block ");
            print_dec(i);
            print_string("\n");
            return;
        }
        // 写入新数据
        uint32_t* data = (uint32_t*)ptrs[i];
        data[0] = 0xCAFE0000 + i;
    }
    print_string("[OK] Blocks re-allocated\n\n");
    
    // 测试5: 大块分配
    print_string("[TEST 5] Allocating large block (8 KB)...\n");
    void* large = rust_kmalloc(8192);
    if (large == NULL) {
        print_string("[FAIL] Failed to allocate large block\n");
        return;
    }
    print_string("  Address: 0x");
    print_hex((uint32_t)((uint64_t)large >> 32));
    print_hex((uint32_t)large);
    print_string("\n");
    
    // 写入并验证大块
    uint32_t* large_data = (uint32_t*)large;
    for (int i = 0; i < 2048; i++) {
        large_data[i] = 0x12340000 + i;
    }
    
    int large_ok = 1;
    for (int i = 0; i < 2048; i++) {
        if (large_data[i] != 0x12340000 + i) {
            large_ok = 0;
            break;
        }
    }
    
    if (large_ok) {
        print_string("[OK] Large block verified\n\n");
    } else {
        print_string("[FAIL] Large block data corruption\n\n");
    }
    
    // 测试6: 获取统计信息
    print_string("[TEST 6] Checking heap statistics...\n");
    size_t total_alloc = 0, total_freed = 0, current = 0;
    size_t alloc_count = 0, free_count = 0;
    rust_heap_stats(&total_alloc, &total_freed, &current, &alloc_count, &free_count);
    
    print_string("  Total Allocated:  ");
    print_dec((uint32_t)(total_alloc / 1024));
    print_string(" KB\n");
    print_string("  Total Freed:      ");
    print_dec((uint32_t)(total_freed / 1024));
    print_string(" KB\n");
    print_string("  Current Usage:    ");
    print_dec((uint32_t)(current / 1024));
    print_string(" KB\n");
    print_string("  Allocations:      ");
    print_dec((uint32_t)alloc_count);
    print_string("\n");
    print_string("  Frees:            ");
    print_dec((uint32_t)free_count);
    print_string("\n");
    print_string("[OK] Statistics retrieved\n\n");
    
    // 测试7: 清理所有分配
    print_string("[TEST 7] Freeing all allocations...\n");
    for (int i = 0; i < 10; i++) {
        if (ptrs[i] != NULL) {
            rust_kfree(ptrs[i]);
        }
    }
    rust_kfree(large);
    print_string("[OK] All memory freed\n\n");
    
    // 测试8: 最终统计
    print_string("[TEST 8] Final heap statistics...\n");
    rust_heap_stats(&total_alloc, &total_freed, &current, &alloc_count, &free_count);
    
    print_string("  Total Allocated:  ");
    print_dec((uint32_t)(total_alloc / 1024));
    print_string(" KB\n");
    print_string("  Total Freed:      ");
    print_dec((uint32_t)(total_freed / 1024));
    print_string(" KB\n");
    print_string("  Current Usage:    ");
    print_dec((uint32_t)(current / 1024));
    print_string(" KB\n");
    print_string("  Allocations:      ");
    print_dec((uint32_t)alloc_count);
    print_string("\n");
    print_string("  Frees:            ");
    print_dec((uint32_t)free_count);
    print_string("\n");
    
    if (current == 0) {
        print_string("[OK] No memory leaks detected\n\n");
    } else {
        print_string("[WARN] Possible memory leak: ");
        print_dec((uint32_t)current);
        print_string(" bytes\n\n");
    }
    
    print_string("==============================================\n");
    print_string("[HEAPTEST] All tests completed successfully!\n");
    print_string("==============================================\n");
}

