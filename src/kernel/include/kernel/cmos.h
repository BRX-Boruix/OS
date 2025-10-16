// Boruix OS CMOS时间模块头文件
// 处理BIOS主板时间读取

#ifndef BORUIX_CMOS_H
#define BORUIX_CMOS_H

// CMOS时间函数声明
unsigned char read_cmos(unsigned char reg);
unsigned char bcd_to_bin(unsigned char bcd);
void get_current_time(unsigned char* hour, unsigned char* minute, unsigned char* second);
void get_current_date(unsigned char* day, unsigned char* month, unsigned char* year);
void print_two_digits(unsigned char num);
void print_current_time(void);

#endif // BORUIX_CMOS_H
