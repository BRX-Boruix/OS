// Boruix OS Shell - 字符串工具函数
// 提供基本的字符串操作功能

#ifndef SHELL_STRING_H
#define SHELL_STRING_H

// 字符串比较函数
int shell_strcmp(const char* str1, const char* str2);

// 字符串长度函数
int shell_strlen(const char* str);

// 字符串复制函数
void shell_strcpy(char* dest, const char* src);

// 简单的字符串分割函数
char* shell_strtok(char* str, const char* delim);

#endif // SHELL_STRING_H
