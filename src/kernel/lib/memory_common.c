// Boruix OS 通用内存函数实现
// 架构无关的内存操作函数

#include "kernel/memory.h"

// 内存设置函数
void* memset(void* dest, int value, size_t count) {
    unsigned char* ptr = (unsigned char*)dest;
    unsigned char val = (unsigned char)value;
    
    for (size_t i = 0; i < count; i++) {
        ptr[i] = val;
    }
    
    return dest;
}

// 内存复制函数
void* memcpy(void* dest, const void* src, size_t count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
    
    return dest;
}

// 内存比较函数
int memcmp(const void* ptr1, const void* ptr2, size_t count) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    
    for (size_t i = 0; i < count; i++) {
        if (p1[i] < p2[i]) return -1;
        if (p1[i] > p2[i]) return 1;
    }
    
    return 0;
}
