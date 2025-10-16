// Boruix OS 内核类型定义
// 定义基本数据类型和常量

#ifndef BORUIX_TYPES_H
#define BORUIX_TYPES_H

// 基本数据类型
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef signed char    int8_t;
typedef signed short   int16_t;
typedef signed int     int32_t;

// 布尔类型
typedef enum {
    false = 0,
    true = 1
} bool;

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
