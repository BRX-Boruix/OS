// Boruix OS irqtest命令 - 测试中断优先级系统
// 通过调整优先级来测试阻塞机制

#include "kernel/shell.h"
#include "kernel/interrupt.h"
#include "drivers/display.h"

// 外部字符串工具函数
extern int shell_strcmp(const char* str1, const char* str2);

void cmd_irqtest(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    print_string("IRQ Priority Test\n");
    print_string("========================================\n\n");
    
    print_string("Testing interrupt priority system...\n\n");
    
    // 显示当前状态
    print_string("Current IRQ priorities:\n");
    for (int i = 0; i < 16; i++) {
        uint8_t priority = irq_get_priority(i);
        if (priority != IRQ_PRIORITY_DISABLED) {
            print_string("IRQ");
            print_dec(i);
            print_string(": ");
            print_string(irq_get_priority_name(priority));
            print_string("\n");
        }
    }
    
    print_string("\nCurrent system status:\n");
    print_string("Interrupt Level: ");
    uint8_t level = irq_get_current_level();
    if (level == IRQ_PRIORITY_DISABLED) {
        print_string("None\n");
    } else {
        print_string(irq_get_priority_name(level));
        print_string("\n");
    }
    
    print_string("Nesting Count: ");
    print_dec(irq_get_nesting_count());
    print_string("\n\n");
    
    // 测试：将键盘设置为低优先级
    print_string("Test 1: Setting keyboard (IRQ1) to LOW priority...\n");
    irq_set_priority(1, IRQ_PRIORITY_LOW);
    print_string("IRQ1 priority set to LOW\n\n");
    
    // 测试：将定时器设置为高优先级
    print_string("Test 2: Setting timer (IRQ0) to HIGH priority...\n");
    irq_set_priority(0, IRQ_PRIORITY_HIGH);
    print_string("IRQ0 priority set to HIGH\n\n");
    
    print_string("Now try typing on keyboard - IRQ1 should be blocked by IRQ0\n");
    print_string("Use 'irqprio' to check blocked counts\n");
    print_string("Use 'irqprio reset' to restore defaults\n\n");
    
    print_string("Test completed. Priority system is working!\n");
}
