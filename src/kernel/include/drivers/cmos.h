// Boruix OS CMOS时间模块头文件
// 处理BIOS主板时间读取

#ifndef BORUIX_CMOS_H
#define BORUIX_CMOS_H

#include "kernel/types.h"

// CMOS时间函数声明
uint8_t read_cmos(uint8_t reg);
uint8_t bcd_to_bin(uint8_t bcd);
void get_current_time(uint8_t* hour, uint8_t* minute, uint8_t* second);
void get_current_date(uint8_t* day, uint8_t* month, uint8_t* year);
void print_two_digits(uint8_t num);
void print_current_time(void);

#endif // BORUIX_CMOS_H
