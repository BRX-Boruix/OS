// Boruix OS irqprio命令 - 中断优先级管理
// 显示和设置IRQ优先级

#include "kernel/shell.h"
#include "kernel/interrupt.h"
#include "drivers/display.h"

// 外部字符串工具函数
extern int shell_strcmp(const char* str1, const char* str2);

// 辅助函数：字符串转整数
static int str_to_int(const char* str) {
    int result = 0;
    int i = 0;
    
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return result;
}

// 显示帮助信息
static void show_help(void) {
    print_string("Usage: irqprio [OPTION] [IRQ] [PRIORITY]\n\n");
    print_string("Options:\n");
    print_string("  (no args)          Show all IRQ priorities\n");
    print_string("  set IRQ PRIORITY   Set IRQ priority\n");
    print_string("  reset              Reset all priorities to default\n");
    print_string("  status             Show priority system status\n");
    print_string("  help               Show this help message\n\n");
    print_string("Priority levels:\n");
    print_string("  0 - Critical (highest)\n");
    print_string("  1 - High\n");
    print_string("  2 - Normal\n");
    print_string("  3 - Low\n\n");
    print_string("Examples:\n");
    print_string("  irqprio              # Show all priorities\n");
    print_string("  irqprio set 1 0      # Set keyboard to critical\n");
    print_string("  irqprio reset        # Reset to defaults\n");
}

// 显示所有IRQ优先级
static void show_all_priorities(void) {
    print_string("IRQ Priority Configuration\n");
    print_string("========================================\n\n");
    
    const char* irq_names[] = {
        "Timer (PIT)",
        "Keyboard (PS/2)",
        "Cascade (PIC2)",
        "COM2",
        "COM1",
        "LPT2",
        "Floppy Disk",
        "LPT1",
        "RTC",
        "Available",
        "Available",
        "Available",
        "PS/2 Mouse",
        "FPU",
        "Primary ATA",
        "Secondary ATA"
    };
    
    print_string("IRQ  Priority   Blocked  Device\n");
    print_string("---  ---------  -------  ---------------------\n");
    
    for (int i = 0; i < 16; i++) {
        uint8_t priority = irq_get_priority(i);
        uint64_t blocked = irq_get_blocked_count(i);
        
        // IRQ号
        print_string("IRQ");
        if (i < 10) print_char(' ');
        print_dec(i);
        print_string("  ");
        
        // 优先级
        const char* prio_name = irq_get_priority_name(priority);
        print_string(prio_name);
        
        // 补齐空格
        int name_len = 0;
        while (prio_name[name_len]) name_len++;
        for (int j = name_len; j < 9; j++) print_char(' ');
        print_string("  ");
        
        // 被阻塞次数
        if (blocked > 0) {
            print_dec((uint32_t)blocked);
        } else {
            print_char('0');
        }
        print_string("       ");
        
        // 设备名
        print_string(irq_names[i]);
        print_string("\n");
    }
    
    print_string("\n");
}

// 显示优先级系统状态
static void show_status(void) {
    print_string("IRQ Priority System Status\n");
    print_string("========================================\n\n");
    
    uint8_t current_level = irq_get_current_level();
    uint32_t nesting_count = irq_get_nesting_count();
    
    print_string("Current Interrupt Level: ");
    if (current_level == IRQ_PRIORITY_DISABLED) {
        print_string("None (no interrupt executing)\n");
    } else {
        print_string(irq_get_priority_name(current_level));
        print_string(" (");
        print_dec(current_level);
        print_string(")\n");
    }
    
    print_string("Interrupt Nesting Count: ");
    print_dec(nesting_count);
    print_string("\n\n");
    
    // 统计被阻塞的中断
    uint64_t total_blocked = 0;
    for (int i = 0; i < 16; i++) {
        total_blocked += irq_get_blocked_count(i);
    }
    
    print_string("Total Blocked Interrupts: ");
    print_dec((uint32_t)total_blocked);
    print_string("\n\n");
    
    if (total_blocked > 0) {
        print_string("IRQs with blocked interrupts:\n");
        for (int i = 0; i < 16; i++) {
            uint64_t blocked = irq_get_blocked_count(i);
            if (blocked > 0) {
                print_string("  IRQ");
                print_dec(i);
                print_string(": ");
                print_dec((uint32_t)blocked);
                print_string(" blocked\n");
            }
        }
    }
}

// 设置IRQ优先级
static void set_priority(int irq, int priority) {
    if (irq < 0 || irq > 15) {
        print_string("Error: IRQ must be between 0 and 15\n");
        return;
    }
    
    if (priority < 0 || priority > 3) {
        print_string("Error: Priority must be between 0 and 3\n");
        return;
    }
    
    irq_set_priority((uint8_t)irq, (uint8_t)priority);
    
    print_string("IRQ");
    print_dec(irq);
    print_string(" priority set to ");
    print_string(irq_get_priority_name((uint8_t)priority));
    print_string(" (");
    print_dec(priority);
    print_string(")\n");
}

// 重置所有优先级
static void reset_priorities(void) {
    irq_reset_priorities();
    print_string("All IRQ priorities reset to default values\n");
}

void cmd_irqprio(int argc, char** argv) {
    // 无参数 - 显示所有优先级
    if (argc == 1) {
        show_all_priorities();
        return;
    }
    
    // 检查第一个参数
    if (shell_strcmp(argv[1], "help") == 0) {
        show_help();
    } else if (shell_strcmp(argv[1], "status") == 0) {
        show_status();
    } else if (shell_strcmp(argv[1], "reset") == 0) {
        reset_priorities();
    } else if (shell_strcmp(argv[1], "set") == 0) {
        if (argc < 4) {
            print_string("Error: 'set' requires IRQ and priority\n");
            print_string("Usage: irqprio set IRQ PRIORITY\n");
            return;
        }
        
        int irq = str_to_int(argv[2]);
        int priority = str_to_int(argv[3]);
        set_priority(irq, priority);
    } else {
        print_string("Unknown option: ");
        print_string(argv[1]);
        print_string("\n");
        print_string("Use 'irqprio help' for usage information\n");
    }
}

