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
extern uint64_t get_gdt_base(void);
extern uint16_t get_gdt_limit(void);

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
    
    // 注意：Limine已经设置好了GDT和TSS
    // x86_64模式下，TSS主要用于IST（中断栈表）
    // CPU会自动使用当前TSS中的IST条目
    // 我们不需要手动加载TSS，Limine已经加载了
    // 只需要修改当前TSS的IST字段即可
    
    // TODO: 在实际实现中，我们应该：
    // 1. 找到Limine设置的TSS地址
    // 2. 修改其IST[0]字段
    // 3. 或者创建自己的GDT和TSS
    
    // 暂时的解决方案：打印调试信息
    (void)get_gdt_base;  // 避免未使用警告
    (void)tss_load;      // 避免未使用警告
    
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

