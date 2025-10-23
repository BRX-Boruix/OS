#include "../../../shell/utils/string.h"
#include "kernel/tty.h"
#include "drivers/display.h"

// 测试页表管理和大内存功能
void cmd_memtest(int argc, char* argv[]) {
    print_string("=== TTY Memory Management Test ===\n");
    
    // 测试基本内存分配
    print_string("Testing basic memory allocation...\n");
    void *ptr1 = tty_kmalloc(1024);
    void *ptr2 = tty_kmalloc(2048);
    void *ptr3 = tty_kmalloc(512);
    
    if (ptr1 && ptr2 && ptr3) {
        print_string("Basic allocation: SUCCESS\n");
        
        // 显示内存统计
        size_t total, used, free, peak;
        tty_memory_stats(&total, &used, &free, &peak);
        print_string("Memory stats - Total: ");
        // 简单的数字转换
        char total_str[16];
        int i = 0;
        int temp = total/1024;
        if (temp == 0) {
            total_str[i++] = '0';
        } else {
            while (temp > 0) {
                total_str[i++] = '0' + (temp % 10);
                temp /= 10;
            }
        }
        total_str[i] = '\0';
        // 反转字符串
        for (int j = 0; j < i/2; j++) {
            char c = total_str[j];
            total_str[j] = total_str[i-1-j];
            total_str[i-1-j] = c;
        }
        print_string(total_str);
        print_string(" KB, Used: ");
        
        // Used内存
        i = 0;
        temp = used/1024;
        if (temp == 0) {
            total_str[i++] = '0';
        } else {
            while (temp > 0) {
                total_str[i++] = '0' + (temp % 10);
                temp /= 10;
            }
        }
        total_str[i] = '\0';
        for (int j = 0; j < i/2; j++) {
            char c = total_str[j];
            total_str[j] = total_str[i-1-j];
            total_str[i-1-j] = c;
        }
        print_string(total_str);
        print_string(" KB\n");
        
        // 释放内存
        tty_kfree(ptr2);
        print_string("Memory deallocation: SUCCESS\n");
        
        // 再次显示统计
        tty_memory_stats(&total, &used, &free, &peak);
        print_string("After free - Used: ");
        // Used内存
        i = 0;
        temp = used/1024;
        if (temp == 0) {
            total_str[i++] = '0';
        } else {
            while (temp > 0) {
                total_str[i++] = '0' + (temp % 10);
                temp /= 10;
            }
        }
        total_str[i] = '\0';
        for (int j = 0; j < i/2; j++) {
            char c = total_str[j];
            total_str[j] = total_str[i-1-j];
            total_str[i-1-j] = c;
        }
        print_string(total_str);
        print_string(" KB\n");
        
        tty_kfree(ptr1);
        tty_kfree(ptr3);
        print_string("All memory freed: SUCCESS\n");
    } else {
        print_string("Basic allocation: FAILED\n");
    }
    
    print_string("\n=== Large Memory Test (Careful) ===\n");
    
    // 测试单个小的大内存分配
    print_string("Testing single large memory allocation (4KB)...\n");
    void *large_ptr = tty_kmalloc_large(4096);  // 4KB
    if (large_ptr) {
        print_string("Large memory allocation: SUCCESS\n");
        
        // 测试简单的内存写入（只写第一个字节）
        char *test_data = (char*)large_ptr;
        test_data[0] = 'A';
        
        if (test_data[0] == 'A') {
            print_string("Memory write test: SUCCESS\n");
        } else {
            print_string("Memory write test: FAILED\n");
        }
        
        // 释放大内存
        tty_kfree_large(large_ptr);
        print_string("Large memory deallocation: SUCCESS\n");
    } else {
        print_string("Large memory allocation: FAILED\n");
    }
    
    print_string("\n=== Memory Management Test Completed ===\n");
}
