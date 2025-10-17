// Boruix OS Shell - 字符串工具函数实现
// 提供基本的字符串操作功能

#include "string.h"
#include "kernel/types.h"

// 字符串比较函数
int shell_strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// 字符串长度函数
int shell_strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

// 字符串复制函数
void shell_strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

// 简单的字符串分割函数
char* shell_strtok(char* str, const char* delim) {
    static char* last = NULL;
    if (str) last = str;
    if (!last) return NULL;
    
    // 跳过分隔符
    while (*last && *last == *delim) last++;
    if (!*last) return NULL;
    
    char* start = last;
    // 找到下一个分隔符
    while (*last && *last != *delim) last++;
    if (*last) {
        *last = '\0';
        last++;
    }
    
    return start;
}
