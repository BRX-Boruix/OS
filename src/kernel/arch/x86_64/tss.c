// Boruix OS x86_64 TSS (Task State Segment) 实现
// 用于双重错误处理的独立栈

#include "kernel/types.h"
#include "drivers/display.h"

// x86_64 TSS结构（简化版，只包含必要字段）
typedef struct {
    uint32_t reserved0;
    uint64_t rsp[3];        // RSP0, RSP1, RSP2 (特权级栈指针)
    uint64_t reserved1;
    uint64_t ist[7];        // IST1-IST7 (中断栈表)
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed)) tss_t;

// 全局TSS实例
static tss_t tss;

// 双重错误独立栈（4KB）
#define DOUBLE_FAULT_STACK_SIZE 4096
static uint8_t double_fault_stack[DOUBLE_FAULT_STACK_SIZE] __attribute__((aligned(16)));

// GDT中的TSS描述符
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed)) tss_descriptor_t;

// 外部汇编函数
extern void tss_load(uint16_t selector);

// 初始化TSS
void tss_init(void) {
    // 清零TSS结构
    for (uint32_t i = 0; i < sizeof(tss_t); i++) {
        ((uint8_t*)&tss)[i] = 0;
    }
    
    // 设置IST1为双重错误栈（栈顶地址）
    tss.ist[0] = (uint64_t)&double_fault_stack[DOUBLE_FAULT_STACK_SIZE];
    
    // 设置其他IST为0（未使用）
    for (int i = 1; i < 7; i++) {
        tss.ist[i] = 0;
    }
    
    // 设置IOMAP基址（指向TSS末尾，表示没有I/O权限位图）
    tss.iomap_base = sizeof(tss_t);
    
    // 注意：TSS将由GDT模块加载
    // 这里只需要初始化TSS结构体
    // gdt_init()会调用tss_get_base()获取TSS地址并设置到GDT中
    (void)tss_load;  // 避免未使用警告（tss_load由gdt_init调用）
    
    print_string("[TSS] Task State Segment initialized\n");
    print_string("[TSS] Double Fault Stack at: 0x");
    print_hex((uint32_t)((uint64_t)double_fault_stack >> 32));
    print_hex((uint32_t)double_fault_stack);
    print_string("\n");
}

// 获取TSS基址（用于调试）
uint64_t tss_get_base(void) {
    return (uint64_t)&tss;
}

// 获取双重错误栈地址（用于调试）
uint64_t tss_get_double_fault_stack(void) {
    return (uint64_t)&double_fault_stack[DOUBLE_FAULT_STACK_SIZE];
}

