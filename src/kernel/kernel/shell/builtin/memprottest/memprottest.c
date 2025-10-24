#include "kernel/kernel.h"
#include "kernel/serial_debug.h"
#include "drivers/display.h"
#include "memprottest.h"

// 外部Rust函数声明
extern uint64_t rust_alloc_page(void);
extern void rust_free_page(uint64_t page_addr);
extern int rust_map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);
extern uint64_t rust_unmap_page(uint64_t virtual_addr);
extern int rust_set_page_readonly(uint64_t virt_addr);
extern int rust_set_page_readwrite(uint64_t virt_addr);
extern int rust_set_page_no_execute(uint64_t virt_addr);
extern int rust_get_page_flags(uint64_t virt_addr, _Bool *present, _Bool *writable, _Bool *user, _Bool *executable);

// 页表标志位
#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITABLE   (1 << 1)

#define TEST_VIRT_BASE  0xFFFFFFFF91000000ULL

void cmd_memprottest(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    print_string("[MEMPROTTEST] Starting memory protection test...\n");
    
    // 测试1: 分配并映射测试页面
    print_string("[TEST 1] Allocating and mapping test page...\n");
    uint64_t phys_page = rust_alloc_page();
    if (phys_page == 0) {
        print_string("[FAIL] Failed to allocate physical page\n");
        return;
    }
    
    uint64_t test_virt = TEST_VIRT_BASE;
    int result = rust_map_page(test_virt, phys_page, PAGE_WRITABLE);
    if (result != 0) {
        print_string("[FAIL] Failed to map page\n");
        rust_free_page(phys_page);
        return;
    }
    
    print_string("  Virtual:  0x");
    print_hex((uint32_t)(test_virt >> 32));
    print_hex((uint32_t)test_virt);
    print_string("\n");
    print_string("  Physical: 0x");
    print_hex((uint32_t)(phys_page >> 32));
    print_hex((uint32_t)phys_page);
    print_string("\n");
    print_string("[OK] Page mapped with RW permissions\n");
    
    // 测试2: 写入初始数据
    print_string("[TEST 2] Writing initial data (read-write mode)...\n");
    volatile uint32_t *ptr = (volatile uint32_t *)test_virt;
    ptr[0] = 0x12345678;
    ptr[1] = 0xABCDEF00;
    
    if (ptr[0] == 0x12345678 && ptr[1] == 0xABCDEF00) {
        print_string("  Data[0]: 0x");
        print_hex(ptr[0]);
        print_string("\n");
        print_string("  Data[1]: 0x");
        print_hex(ptr[1]);
        print_string("\n");
        print_string("[OK] Data written successfully\n");
    } else {
        print_string("[FAIL] Data verification failed\n");
    }
    
    // 测试3: 查询当前保护标志
    print_string("[TEST 3] Checking current page protection...\n");
    _Bool present = 0, writable = 0, user = 0, executable = 0;
    result = rust_get_page_flags(test_virt, &present, &writable, &user, &executable);
    if (result != 0) {
        print_string("[FAIL] Failed to get page flags\n");
    } else {
        print_string("  Present:    ");
        print_string(present ? "Yes\n" : "No\n");
        print_string("  Writable:   ");
        print_string(writable ? "Yes\n" : "No\n");
        print_string("  User:       ");
        print_string(user ? "Yes\n" : "No\n");
        print_string("  Executable: ");
        print_string(executable ? "Yes\n" : "No\n");
        print_string("[OK] Flags retrieved successfully\n");
    }
    
    // 测试4: 设置为只读
    print_string("[TEST 4] Setting page to READ-ONLY...\n");
    result = rust_set_page_readonly(test_virt);
    if (result != 0) {
        print_string("[FAIL] Failed to set page read-only\n");
    } else {
        print_string("[OK] Page set to read-only\n");
        
        // 验证标志
        rust_get_page_flags(test_virt, &present, &writable, &user, &executable);
        print_string("  Writable:   ");
        print_string(writable ? "Yes (ERROR!)\n" : "No (CORRECT)\n");
    }
    print_string("\n");
    
    // 测试5: 读取只读页面的数据
    print_string("[TEST 5] Reading from read-only page...\n");
    uint32_t val0 = ptr[0];
    uint32_t val1 = ptr[1];
    
    if (val0 == 0x12345678 && val1 == 0xABCDEF00) {
        print_string("[OK] Read successful, data intact:\n");
        print_string("  Data[0]: 0x");
        print_hex(val0);
        print_string("\n");
        print_string("  Data[1]: 0x");
        print_hex(val1);
        print_string("\n");
    } else {
        print_string("[FAIL] Data corrupted!\n");
    }
    print_string("\n");
    
    // 测试7: 恢复为可读写
    print_string("[TEST 7] Restoring page to READ-WRITE...\n");
    result = rust_set_page_readwrite(test_virt);
    if (result != 0) {
        print_string("[FAIL] Failed to set page read-write\n");
    } else {
        print_string("[OK] Page set to read-write\n");
        
        // 验证标志
        rust_get_page_flags(test_virt, &present, &writable, &user, &executable);
        print_string("  Writable:   ");
        print_string(writable ? "Yes (CORRECT)\n" : "No (ERROR!)\n");
    }
    print_string("\n");
    
    // 测试8: 写入新数据
    print_string("[TEST 8] Writing new data after restoring RW...\n");
    ptr[0] = 0xDEADBEEF;
    ptr[1] = 0xCAFEBABE;
    
    if (ptr[0] == 0xDEADBEEF && ptr[1] == 0xCAFEBABE) {
        print_string("[OK] New data written successfully:\n");
        print_string("  Data[0]: 0x");
        print_hex(ptr[0]);
        print_string("\n");
        print_string("  Data[1]: 0x");
        print_hex(ptr[1]);
        print_string("\n");
    } else {
        print_string("[FAIL] Write operation failed\n");
    }
    print_string("\n");
    
    // 测试9: 设置为不可执行
    print_string("[TEST 9] Setting page to NO-EXECUTE (NX)...\n");
    result = rust_set_page_no_execute(test_virt);
    if (result != 0) {
        print_string("[FAIL] Failed to set page no-execute\n");
    } else {
        print_string("[OK] Page set to no-execute (NX bit set)\n");
        
        // 验证标志
        rust_get_page_flags(test_virt, &present, &writable, &user, &executable);
        print_string("  Executable: ");
        print_string(executable ? "Yes (ERROR!)\n" : "No (CORRECT)\n");
        print_string("  [INFO] Executing code on NX page -> Exception #14\n");
    }
    print_string("\n");
    
    // 测试10: 最终状态检查
    print_string("[TEST 10] Final page state verification...\n");
    rust_get_page_flags(test_virt, &present, &writable, &user, &executable);
    print_string("  Present:    ");
    print_string(present ? "Yes\n" : "No\n");
    print_string("  Writable:   ");
    print_string(writable ? "Yes\n" : "No\n");
    print_string("  User:       ");
    print_string(user ? "Yes\n" : "No\n");
    print_string("  Executable: ");
    print_string(executable ? "Yes\n" : "No\n");
    print_string("[OK] Final state verified\n");
    
    // 测试11: 清理
    print_string("[TEST 11] Cleaning up resources...\n");
    rust_unmap_page(test_virt);
    rust_free_page(phys_page);
    print_string("[OK] Page unmapped and physical memory freed\n");
}
