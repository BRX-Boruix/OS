#ifndef BORUIX_SHELL_H
#define BORUIX_SHELL_H

#include "kernel/types.h"

// Shell配置
#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16
#define SHELL_PROMPT "boruix> "

// 命令结构
typedef struct {
    const char* name;
    const char* description;
    void (*function)(int argc, char* argv[]);
} shell_command_t;

// Shell函数声明
void shell_main(void);
void shell_init(void);
void shell_process_command(const char* input);
void shell_print_prompt(void);

// 内置命令
void cmd_help(int argc, char* argv[]);
void cmd_clear(int argc, char* argv[]);
void cmd_echo(int argc, char* argv[]);
void cmd_time(int argc, char* argv[]);
void cmd_info(int argc, char* argv[]);
void cmd_reboot(int argc, char* argv[]);

// 字符串工具函数
int shell_strcmp(const char* str1, const char* str2);
int shell_strlen(const char* str);
void shell_strcpy(char* dest, const char* src);
char* shell_strtok(char* str, const char* delim);

#endif // BORUIX_SHELL_H
