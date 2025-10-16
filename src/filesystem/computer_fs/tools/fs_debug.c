#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../vfs_core/computer_vfs.h"
#include "../device_layer/device_interface.h"

/**
 * Computer:/ 文件系统调试工具
 */

void print_usage(const char *program_name) {
    printf("用法: %s [选项]\n", program_name);
    printf("选项:\n");
    printf("  -i, --init      初始化文件系统\n");
    printf("  -l, --list      列出所有设备\n");
    printf("  -t, --tree      显示目录树\n");
    printf("  -s, --status    显示系统状态\n");
    printf("  -h, --help      显示此帮助信息\n");
}

void print_device_list(void) {
    printf("=== 设备列表 ===\n");
    // TODO: 实现设备列表显示
    printf("(暂未实现)\n");
}

void print_directory_tree(void) {
    printf("=== Computer:/ 目录树 ===\n");
    printf("computer:/\n");
    printf("├── cpu/\n");
    printf("│   ├── core0/\n");
    printf("│   │   ├── freq\n");
    printf("│   │   ├── temp\n");
    printf("│   │   └── usage\n");
    printf("│   └── info\n");
    printf("├── mem/\n");
    printf("│   ├── total\n");
    printf("│   ├── free\n");
    printf("│   └── used\n");
    printf("├── disk/\n");
    printf("├── SYSTEM/\n");
    printf("└── ...\n");
}

void print_system_status(void) {
    printf("=== 系统状态 ===\n");
    printf("文件系统: Computer:/ VFS\n");
    printf("状态: 开发中\n");
    printf("版本: 0.1.0-alpha\n");
    // TODO: 显示实际的系统状态
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--init") == 0) {
            printf("初始化 Computer:/ 文件系统...\n");
            if (computer_vfs_init() == 0) {
                printf("初始化成功!\n");
                computer_vfs_cleanup();
            } else {
                printf("初始化失败!\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0) {
            print_device_list();
        }
        else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tree") == 0) {
            print_directory_tree();
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--status") == 0) {
            print_system_status();
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
        }
        else {
            printf("未知选项: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    return 0;
}
