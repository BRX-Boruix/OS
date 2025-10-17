// Boruix OS Shell - 命令处理
// 处理命令解析和执行

#ifndef COMMAND_H
#define COMMAND_H

#include "../utils/string.h"

// 命令函数类型定义
typedef void (*shell_command_func_t)(int argc, char* argv[]);

// 命令结构体
typedef struct {
    const char* name;
    const char* description;
    shell_command_func_t function;
} shell_command_t;

// 处理命令
void shell_process_command(const char* input);

// 获取命令表
shell_command_t* shell_get_commands(void);

#endif // COMMAND_H
