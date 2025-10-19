#ifndef BORUIX_SHELL_H
#define BORUIX_SHELL_H

#include "kernel/types.h"

// Shell配置
#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16
#define SHELL_PROMPT "Boruix> "

// 安全的参数访问宏
#define SAFE_ARGV(argc, argv, index) \
    ((index) < (argc) ? (argv)[(index)] : NULL)

#define SAFE_ARGV_STR(argc, argv, index, default_str) \
    ((index) < (argc) ? (argv)[(index)] : (default_str))

// 安全的参数检查宏
#define HAS_ARG(argc, index) ((index) < (argc))
#define ARGC_CHECK(argc, min_args) ((argc) >= (min_args))

// 安全的参数访问函数
static inline const char* safe_argv(int argc, char* argv[], int index) {
    return (index < argc) ? argv[index] : NULL;
}

static inline const char* safe_argv_str(int argc, char* argv[], int index, const char* default_str) {
    return (index < argc) ? argv[index] : default_str;
}

static inline int has_arg(int argc, int index) {
    return (index < argc);
}

static inline int argc_check(int argc, int min_args) {
    return (argc >= min_args);
}

// Shell函数声明
void shell_main(void);
void shell_init(void);
void shell_print_prompt(void);

#endif // BORUIX_SHELL_H
