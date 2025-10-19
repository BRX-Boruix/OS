// Boruix OS 类型定义
// 提供基础数据类型定义（仅x86_64）

#ifndef BORUIX_TYPES_H
#define BORUIX_TYPES_H

// 基础类型定义（x86_64）
typedef unsigned long size_t;
typedef unsigned long uintptr_t;
typedef unsigned long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef signed long int64_t;
typedef signed int int32_t;
typedef signed short int16_t;
typedef signed char int8_t;

// 布尔类型
typedef enum { false = 0, true = 1 } bool;

// NULL定义
#ifndef NULL
#define NULL ((void*)0)
#endif

// 屏幕参数
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define VIDEO_MEMORY 0xB8000

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

#endif // BORUIX_TYPES_H
