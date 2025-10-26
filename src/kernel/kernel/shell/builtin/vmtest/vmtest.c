// Boruix OS vmtest命令 - 虚拟内存隔离测试

#include "kernel/types.h"
#include "kernel/process.h"
#include "drivers/display.h"
#include "kernel/shell.h"

extern void serial_puts(const char*);
extern void serial_put_hex(uint64_t);
extern uint64_t rust_get_process_cr3(uint32_t pid);

// 简单的测试进程
void vmtest_process1(void) {
    serial_puts("[VMTEST-P1] Process started!\n");
    for (volatile int i = 0; i < 10; i++) {
        serial_puts("[VMTEST-P1] Tick\n");
    }
    serial_puts("[VMTEST-P1] Process done\n");
}

// vmtest命令
void builtin_vmtest(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    print_string("=== VM Isolation Test ===\n");
    serial_puts("[VMTEST] Test starting\n");
    
    print_string("Process function at: 0x");
    print_hex((uint64_t)vmtest_process1);
    print_string("\n");
    
    serial_puts("[VMTEST] Creating process...\n");
    uint32_t pid1 = rust_create_process(
        (const uint8_t*)"vmtest1", 
        7, 
        (uintptr_t)vmtest_process1, 
        1
    );
    
    if (pid1 == 0) {
        print_string("ERROR: Failed to create process\n");
        serial_puts("[VMTEST] ERROR: Process creation failed\n");
        return;
    }
    
    print_string("Created PID=");
    print_dec(pid1);
    print_string("\n");
    
    uint64_t cr3 = rust_get_process_cr3(pid1);
    print_string("CR3=0x");
    print_hex(cr3);
    print_string("\n");
    
    serial_puts("[VMTEST] Process created, should run soon\n");
    print_string("Check serial output for process messages\n");
}
