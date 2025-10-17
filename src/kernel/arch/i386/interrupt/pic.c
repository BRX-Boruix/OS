// Boruix OS i386 PIC 8259A初始化

#include "kernel/types.h"

// PIC端口
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

// PIC命令
#define PIC_EOI 0x20
#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

// I/O端口操作
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

// 初始化PIC
void pic_init(void) {
    // 保存当前屏蔽字
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    
    // ICW1: 开始初始化
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    
    // ICW2: 设置中断向量偏移
    outb(PIC1_DATA, 32);  // Master PIC: IRQ 0-7 映射到 INT 32-39
    io_wait();
    outb(PIC2_DATA, 40);  // Slave PIC: IRQ 8-15 映射到 INT 40-47
    io_wait();
    
    // ICW3: 设置级联
    outb(PIC1_DATA, 0x04);  // Master: IRQ2连接slave
    io_wait();
    outb(PIC2_DATA, 0x02);  // Slave: 连接到master的IRQ2
    io_wait();
    
    // ICW4: 设置8086模式
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();
    
    // 恢复屏蔽字
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

// 发送EOI
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

// 设置IRQ屏蔽
void pic_set_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}

// 清除IRQ屏蔽
void pic_clear_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}
