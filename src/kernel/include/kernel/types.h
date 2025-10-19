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

#endif // BORUIX_TYPES_H
