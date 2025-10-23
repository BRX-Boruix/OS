// Boruix OS x86_64 GDT (Global Descriptor Table) 实现
// 创建自己的GDT以支持TSS和双重错误处理

#include "kernel/types.h"
#include "drivers/display.h"

// GDT条目结构（标准x86_64格式）
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

// TSS描述符（x86_64需要16字节）
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

// GDT指针结构
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_ptr_t;

// GDT表（8个条目）
// 0: NULL descriptor
// 1: Kernel Code (64-bit)
// 2: Kernel Data
// 3: User Code (64-bit)
// 4: User Data
// 5: TSS (16字节，占用2个槽位)
#define GDT_ENTRIES 7
static gdt_entry_t gdt[GDT_ENTRIES];
static gdt_ptr_t gdt_ptr;

// TSS选择子（第5项，偏移0x28）
#define TSS_SELECTOR 0x28

// 外部函数
extern void gdt_load(uint64_t gdt_ptr_addr);
extern void gdt_reload_segments(void);
extern void tss_load(uint16_t selector);
extern uint64_t tss_get_base(void);

// 设置标准GDT条目
static void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_mid = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

// 设置TSS描述符（16字节）
static void gdt_set_tss(int num, uint64_t base, uint32_t limit) {
    tss_descriptor_t* tss_desc = (tss_descriptor_t*)&gdt[num];
    
    tss_desc->limit_low = limit & 0xFFFF;
    tss_desc->base_low = base & 0xFFFF;
    tss_desc->base_mid = (base >> 16) & 0xFF;
    tss_desc->access = 0x89;  // Present, DPL=0, Type=Available TSS (64-bit)
    tss_desc->granularity = 0x00;  // 字节粒度
    tss_desc->base_high = (base >> 24) & 0xFF;
    tss_desc->base_upper = (base >> 32) & 0xFFFFFFFF;
    tss_desc->reserved = 0;
}

// 初始化GDT
void gdt_init(void) {
    print_string("[GDT] Initializing Global Descriptor Table...\n");
    
    // 0: NULL descriptor
    gdt_set_entry(0, 0, 0, 0, 0);
    
    // 1: Kernel Code Segment (64-bit)
    // Base = 0, Limit = 0xFFFFF
    // Access = 0x9A (Present, Ring 0, Code, Execute/Read)
    // Granularity = 0xA0 (64-bit, 4KB pages)
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xA0);
    
    // 2: Kernel Data Segment
    // Base = 0, Limit = 0xFFFFF
    // Access = 0x92 (Present, Ring 0, Data, Read/Write)
    // Granularity = 0xC0 (32-bit, 4KB pages)
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xC0);
    
    // 3: User Code Segment (64-bit)
    // Access = 0xFA (Present, Ring 3, Code, Execute/Read)
    // Granularity = 0xA0 (64-bit, 4KB pages)
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0xA0);
    
    // 4: User Data Segment
    // Access = 0xF2 (Present, Ring 3, Data, Read/Write)
    // Granularity = 0xC0 (32-bit, 4KB pages)
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0xC0);
    
    // 5-6: TSS (16字节，占用2个槽位)
    uint64_t tss_base = tss_get_base();
    uint32_t tss_limit = 104 - 1;  // sizeof(tss_t) = 104
    gdt_set_tss(5, tss_base, tss_limit);
    
    // 设置GDT指针
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint64_t)&gdt;
    
    // 加载GDT
    gdt_load((uint64_t)&gdt_ptr);
    
    // 重新加载段寄存器
    gdt_reload_segments();
    
    // 加载TSS
    tss_load(TSS_SELECTOR);
    
    print_string("[GDT] Global Descriptor Table initialized\n");
    print_string("[GDT] TSS loaded at selector 0x");
    print_hex(TSS_SELECTOR);
    print_string("\n");
}

// 获取GDT基址
uint64_t gdt_get_base(void) {
    return gdt_ptr.base;
}

// 获取TSS选择子
uint16_t gdt_get_tss_selector(void) {
    return TSS_SELECTOR;
}

