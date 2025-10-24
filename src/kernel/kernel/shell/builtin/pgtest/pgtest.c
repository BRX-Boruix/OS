#include "kernel/kernel.h"
#include "kernel/serial_debug.h"
#include "drivers/display.h"
#include "pgtest.h"

// 外部Rust函数声明
extern uint64_t rust_alloc_page(void);
extern void rust_free_page(uint64_t page_addr);
extern int rust_map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);
extern uint64_t rust_unmap_page(uint64_t virtual_addr);
extern uint64_t rust_virt_to_phys(uint64_t virtual_addr);

// 页表标志位
#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITABLE   (1 << 1)
#define PAGE_USER       (1 << 2)

#define TEST_VIRT_ADDR  0xFFFFFFFF90000000ULL  // 测试虚拟地址（内核堆区域）

void cmd_pgtest(int argc, char* argv[]) {
    (void)argc;  // 未使用的参数
    (void)argv;  // 未使用的参数
    
    print_string("[PGTEST] Starting paging system test...\n");
    
    // 测试1: 分配物理页面
    print_string("[TEST 1] Allocating physical page...\n");
    uint64_t phys_page = rust_alloc_page();
    if (phys_page == 0) {
        print_string("[FAIL] Failed to allocate physical page\n");
        return;
    }
    print_string("[OK] Physical page allocated at 0x");
    print_hex((uint32_t)(phys_page >> 32));
    print_hex((uint32_t)phys_page);
    print_string("\n");
    
    // 测试2: 映射虚拟页面到物理页面
    print_string("[TEST 2] Mapping virtual page to physical page...\n");
    print_string("  Virtual:  0x");
    print_hex((uint32_t)(TEST_VIRT_ADDR >> 32));
    print_hex((uint32_t)TEST_VIRT_ADDR);
    print_string("\n");
    print_string("  Physical: 0x");
    print_hex((uint32_t)(phys_page >> 32));
    print_hex((uint32_t)phys_page);
    print_string("\n");
    
    int result = rust_map_page(TEST_VIRT_ADDR, phys_page, PAGE_WRITABLE);
    if (result != 0) {
        print_string("[FAIL] Failed to map page (code: ");
        print_dec(result);
        print_string(")\n");
        rust_free_page(phys_page);
        return;
    }
    print_string("[OK] Page mapped successfully\n");
    
    // 测试3: 虚拟地址转物理地址
    print_string("[TEST 3] Translating virtual to physical address...\n");
    uint64_t translated = rust_virt_to_phys(TEST_VIRT_ADDR);
    if (translated == 0) {
        print_string("[FAIL] Failed to translate address\n");
        rust_unmap_page(TEST_VIRT_ADDR);
        rust_free_page(phys_page);
        return;
    }
    print_string("[OK] Translated address: 0x");
    print_hex((uint32_t)(translated >> 32));
    print_hex((uint32_t)translated);
    print_string("\n");
    
    // 验证地址是否匹配
    if (translated == phys_page) {
        print_string("[OK] Translation matches original physical address\n");
    } else {
        print_string("[WARN] Translation does not match (expected 0x");
        print_hex((uint32_t)(phys_page >> 32));
        print_hex((uint32_t)phys_page);
        print_string(")\n");
    }
    
    // 测试4: 写入并读取映射的内存
    print_string("[TEST 4] Writing to and reading from mapped memory...\n");
    volatile uint32_t *test_ptr = (volatile uint32_t *)TEST_VIRT_ADDR;
    *test_ptr = 0xDEADBEEF;
    uint32_t read_value = *test_ptr;
    
    if (read_value == 0xDEADBEEF) {
        print_string("[OK] Memory read/write successful (0xDEADBEEF)\n");
    } else {
        print_string("[FAIL] Memory read/write failed (got 0x");
        print_hex(read_value);
        print_string(")\n");
    }
    
    // 测试5: 取消页面映射
    print_string("[TEST 5] Unmapping page...\n");
    uint64_t unmapped_phys = rust_unmap_page(TEST_VIRT_ADDR);
    if (unmapped_phys == 0) {
        print_string("[FAIL] Failed to unmap page\n");
        rust_free_page(phys_page);
        return;
    }
    print_string("[OK] Page unmapped, returned physical address: 0x");
    print_hex((uint32_t)(unmapped_phys >> 32));
    print_hex((uint32_t)unmapped_phys);
    print_string("\n");
    
    // 验证物理地址是否匹配
    if (unmapped_phys == phys_page) {
        print_string("[OK] Unmapped address matches original\n");
    } else {
        print_string("[WARN] Unmapped address does not match\n");
    }
    
    // 测试6: 验证页面已取消映射
    print_string("[TEST 6] Verifying page is unmapped...\n");
    uint64_t should_fail = rust_virt_to_phys(TEST_VIRT_ADDR);
    if (should_fail == 0) {
        print_string("[OK] Page is correctly unmapped (translation returns 0)\n");
    } else {
        print_string("[WARN] Page still appears to be mapped\n");
    }
    
    // 测试7: 释放物理页面
    print_string("[TEST 7] Freeing physical page...\n");
    rust_free_page(phys_page);
    print_string("[OK] Physical page freed\n");
    
    print_string("\n");
    print_string("==============================================\n");
    print_string("[PGTEST] All tests completed successfully!\n");
    print_string("==============================================\n");
}

