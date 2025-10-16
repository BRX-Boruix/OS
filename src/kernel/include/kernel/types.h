// Boruix OS 内核类型定义
// 定义基本数据类型和常量

#ifndef BORUIX_TYPES_H
#define BORUIX_TYPES_H

// 架构检测宏
#ifndef __x86_64__
#ifndef __i386__
// 如果没有明确定义架构，根据编译器预定义宏判断
#if defined(__LP64__) || defined(_WIN64)
#define __x86_64__
#else
#define __i386__
#endif
#endif
#endif

// 基本数据类型
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned long long uint64_t;
typedef signed char    int8_t;
typedef signed short   int16_t;
typedef signed int     int32_t;
typedef signed long long int64_t;

// 架构相关类型
#ifdef __x86_64__
typedef uint64_t uintptr_t;  // 64位指针类型
typedef int64_t  intptr_t;   // 64位有符号指针类型
typedef uint64_t size_t;     // 64位大小类型
#else
typedef uint32_t uintptr_t;  // 32位指针类型
typedef int32_t  intptr_t;   // 32位有符号指针类型
typedef uint32_t size_t;     // 32位大小类型
#endif

// 布尔类型
typedef enum {
    false = 0,
    true = 1
} bool;

// NULL指针定义
#ifndef NULL
#define NULL ((void*)0)
#endif

// 颜色定义
#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define MAGENTA 5
#define BROWN 6
#define LIGHT_GRAY 7
#define DARK_GRAY 8
#define LIGHT_BLUE 9
#define LIGHT_GREEN 10
#define LIGHT_CYAN 11
#define LIGHT_RED 12
#define LIGHT_MAGENTA 13
#define YELLOW 14
#define WHITE 15

// 屏幕参数
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define VIDEO_MEMORY 0xB8000

#endif // BORUIX_TYPES_H
