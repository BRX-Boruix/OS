#include "kernel/kernel.h"
#include "kernel/serial_debug.h"
#include "drivers/display.h"
#include "vmmtest.h"

// 外部Rust函数声明
extern uint64_t rust_vmm_allocate(uint64_t size);
extern int rust_vmm_map_and_allocate(uint64_t size, uint64_t *out_virt_addr);
extern void rust_vmm_get_heap_usage(uint64_t *used, uint64_t *total);

void cmd_vmmtest(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    print_string("[VMMTEST] Starting Virtual Memory Manager test...\n");
    print_string("\n");
    
    // 测试1: 获取内核堆使用情况
    print_string("[TEST 1] Checking kernel heap usage...\n");
    uint64_t used = 0, total = 0;
    rust_vmm_get_heap_usage(&used, &total);
    
    print_string("  Heap Start:   0xFFFFFFFF90000000\n");
    print_string("  Heap End:     0xFFFFFFFFA0000000\n");
    print_string("  Total Size:   ");
    print_dec((uint32_t)(total / 1024 / 1024));
    print_string(" MB\n");
    print_string("  Used:         ");
    print_dec((uint32_t)(used / 1024));
    print_string(" KB\n");
    print_string("  Available:    ");
    print_dec((uint32_t)((total - used) / 1024 / 1024));
    print_string(" MB\n");
    print_string("[OK] Heap usage retrieved\n\n");
    
    // 测试2: 分配虚拟地址空间（不映射）
    print_string("[TEST 2] Allocating virtual address space (16 KB)...\n");
    uint64_t virt_addr = rust_vmm_allocate(16 * 1024);
    if (virt_addr == 0) {
        print_string("[FAIL] Failed to allocate virtual address\n");
        return;
    }
    print_string("  Allocated at: 0x");
    print_hex((uint32_t)(virt_addr >> 32));
    print_hex((uint32_t)virt_addr);
    print_string("\n");
    print_string("[OK] Virtual address allocated\n\n");
    
    // 测试3: 分配并映射内存（小块）
    print_string("[TEST 3] Allocating and mapping memory (4 KB)...\n");
    uint64_t mapped_addr = 0;
    int result = rust_vmm_map_and_allocate(4096, &mapped_addr);
    if (result != 0) {
        print_string("[FAIL] Failed to allocate and map memory (code: ");
        print_dec(result);
        print_string(")\n");
        return;
    }
    print_string("  Mapped at:    0x");
    print_hex((uint32_t)(mapped_addr >> 32));
    print_hex((uint32_t)mapped_addr);
    print_string("\n");
    print_string("[OK] Memory allocated and mapped\n\n");
    
    // 测试4: 写入并读取映射的内存
    print_string("[TEST 4] Writing to and reading from mapped memory...\n");
    volatile uint32_t *test_ptr = (volatile uint32_t *)mapped_addr;
    
    // 写入测试模式
    test_ptr[0] = 0x12345678;
    test_ptr[1] = 0xABCDEF00;
    test_ptr[2] = 0xDEADBEEF;
    test_ptr[3] = 0xCAFEBABE;
    
    // 读取并验证
    uint32_t val0 = test_ptr[0];
    uint32_t val1 = test_ptr[1];
    uint32_t val2 = test_ptr[2];
    uint32_t val3 = test_ptr[3];
    
    print_string("  [0]: 0x");
    print_hex(val0);
    if (val0 == 0x12345678) {
        print_string(" [OK]\n");
    } else {
        print_string(" [FAIL]\n");
    }
    
    print_string("  [1]: 0x");
    print_hex(val1);
    if (val1 == 0xABCDEF00) {
        print_string(" [OK]\n");
    } else {
        print_string(" [FAIL]\n");
    }
    
    print_string("  [2]: 0x");
    print_hex(val2);
    if (val2 == 0xDEADBEEF) {
        print_string(" [OK]\n");
    } else {
        print_string(" [FAIL]\n");
    }
    
    print_string("  [3]: 0x");
    print_hex(val3);
    if (val3 == 0xCAFEBABE) {
        print_string(" [OK]\n");
    } else {
        print_string(" [FAIL]\n");
    }
    
    if (val0 == 0x12345678 && val1 == 0xABCDEF00 && 
        val2 == 0xDEADBEEF && val3 == 0xCAFEBABE) {
        print_string("[OK] All memory operations successful\n\n");
    } else {
        print_string("[FAIL] Memory verification failed\n\n");
        return;
    }
    
    // 测试5: 分配较大的内存块
    print_string("[TEST 5] Allocating larger memory block (64 KB)...\n");
    uint64_t large_addr = 0;
    result = rust_vmm_map_and_allocate(64 * 1024, &large_addr);
    if (result != 0) {
        print_string("[FAIL] Failed to allocate large block (code: ");
        print_dec(result);
        print_string(")\n");
        return;
    }
    print_string("  Mapped at:    0x");
    print_hex((uint32_t)(large_addr >> 32));
    print_hex((uint32_t)large_addr);
    print_string("\n");
    print_string("[OK] Large block allocated\n\n");
    
    // 测试6: 检查更新后的堆使用情况
    print_string("[TEST 6] Checking updated heap usage...\n");
    rust_vmm_get_heap_usage(&used, &total);
    
    print_string("  Used:         ");
    print_dec((uint32_t)(used / 1024));
    print_string(" KB\n");
    print_string("  Available:    ");
    print_dec((uint32_t)((total - used) / 1024 / 1024));
    print_string(" MB\n");
    print_string("[OK] Heap usage updated\n\n");
    
    print_string("==============================================\n");
    print_string("[VMMTEST] All tests completed successfully!\n");
    print_string("==============================================\n");
}

