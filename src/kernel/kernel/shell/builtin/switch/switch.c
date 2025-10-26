// switch命令 - 切换用户模式
#include "kernel/shell.h"
#include "drivers/display.h"
#include "../../../shell/utils/string.h"

// 全局变量：当前模式（0=user, 1=super）
static int current_mode = 0;  // 默认为普通用户模式

void cmd_switch(int argc, char** argv) {
    if (argc < 2) {
        print_string("Usage: switch <super|user>\n");
        print_string("Switch between super user and normal user mode\n");
        print_string("\nCurrent mode: ");
        if (current_mode == 1) {
            print_string("SUPER USER\n");
        } else {
            print_string("USER\n");
        }
        return;
    }
    
    if (shell_strcmp(argv[1], "super") == 0) {
        current_mode = 1;
        print_string("\n");
        print_string("========================================\n");
        print_string("  SWITCHED TO SUPER USER MODE\n");
        print_string("========================================\n");
        print_string("Warning: You can now terminate system processes!\n");
        print_string("Terminating PID 0-2 will crash the system!\n");
        print_string("Use with extreme caution.\n\n");
    } else if (shell_strcmp(argv[1], "user") == 0) {
        current_mode = 0;
        print_string("\n");
        print_string("========================================\n");
        print_string("  SWITCHED TO USER MODE\n");
        print_string("========================================\n");
        print_string("System processes are now protected.\n\n");
    } else {
        print_string("Error: Invalid mode. Use 'super' or 'user'\n");
    }
}

// 检查是否为超级用户模式
int is_super_mode(void) {
    return current_mode;
}

