// Boruix OS Shell - 内置命令
// 实现shell的内置命令功能

#ifndef SHELL_BUILTIN_H
#define SHELL_BUILTIN_H

// 内置命令函数声明
void cmd_help(int argc, char* argv[]);
void cmd_clear(int argc, char* argv[]);
void cmd_echo(int argc, char* argv[]);
void cmd_time(int argc, char* argv[]);
void cmd_info(int argc, char* argv[]);
void cmd_reboot(int argc, char* argv[]);

#endif // SHELL_BUILTIN_H
