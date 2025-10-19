// Boruix OS Shell - 命令安全验证工具
// 提供命令参数访问的安全检查

#ifndef COMMAND_SAFETY_H
#define COMMAND_SAFETY_H

#include "kernel/shell.h"

// 命令安全验证宏
#define VALIDATE_COMMAND_ARGS(argc, argv, min_args) \
    do { \
        if (!ARGC_CHECK(argc, min_args)) { \
            print_string("Error: Insufficient arguments. Required: "); \
            print_dec(min_args - 1); \
            print_string(" arguments.\n"); \
            return; \
        } \
    } while(0)

#define VALIDATE_COMMAND_ARGS_WITH_USAGE(argc, argv, min_args, usage_msg) \
    do { \
        if (!ARGC_CHECK(argc, min_args)) { \
            print_string("Error: Insufficient arguments.\n"); \
            print_string("Usage: "); \
            print_string(usage_msg); \
            print_string("\n"); \
            return; \
        } \
    } while(0)

// 安全的参数比较宏
#define SAFE_STRCMP(argc, argv, index, str) \
    (HAS_ARG(argc, index) ? shell_strcmp(SAFE_ARGV_STR(argc, argv, index, ""), str) : -1)

#define SAFE_STRCASECMP(argc, argv, index, str) \
    (HAS_ARG(argc, index) ? shell_strcasecmp(SAFE_ARGV_STR(argc, argv, index, ""), str) : -1)

// 命令安全检查函数
static inline int validate_command_safety(int argc, char* argv[], int min_args) {
    if (!ARGC_CHECK(argc, min_args)) {
        print_string("Error: Command requires at least ");
        print_dec(min_args - 1);
        print_string(" arguments.\n");
        return 0;
    }
    return 1;
}

static inline int validate_command_safety_with_usage(int argc, char* argv[], int min_args, const char* usage) {
    if (!ARGC_CHECK(argc, min_args)) {
        print_string("Error: Insufficient arguments.\n");
        print_string("Usage: ");
        print_string(usage);
        print_string("\n");
        return 0;
    }
    return 1;
}

// 安全的参数处理宏
#define PROCESS_SAFE_ARGS(argc, argv, ...) \
    do { \
        int _argc = (argc); \
        char** _argv = (argv); \
        __VA_ARGS__ \
    } while(0)

// 示例用法：
/*
void cmd_example(int argc, char* argv[]) {
    // 方法1：使用验证宏
    VALIDATE_COMMAND_ARGS(argc, argv, 2);
    
    // 方法2：使用验证函数
    if (!validate_command_safety(argc, argv, 2)) {
        return;
    }
    
    // 方法3：使用安全的字符串比较
    if (SAFE_STRCMP(argc, argv, 1, "help") == 0) {
        print_string("Help message\n");
        return;
    }
    
    // 方法4：使用安全的参数访问
    const char* first_arg = SAFE_ARGV_STR(argc, argv, 1, "");
    const char* second_arg = SAFE_ARGV_STR(argc, argv, 2, "");
    
    // 处理参数...
}
*/

#endif // COMMAND_SAFETY_H
