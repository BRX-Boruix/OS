// Boruix OS CMOS时间模块实现
// 处理BIOS主板时间读取

#include "drivers/cmos.h"
#include "drivers/display.h"
#include "kernel/kernel.h"

// CMOS时钟读取函数
unsigned char read_cmos(unsigned char reg) {
    // 选择CMOS寄存器
    __asm__ volatile ("outb %0, %1" : : "a" (reg), "Nd" (CMOS_ADDRESS));
    // 读取数据
    unsigned char data;
    __asm__ volatile ("inb %1, %0" : "=a" (data) : "Nd" (CMOS_DATA));
    return data;
}

// 将BCD码转换为二进制
unsigned char bcd_to_bin(unsigned char bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// 读取当前时间
void get_current_time(unsigned char* hour, unsigned char* minute, unsigned char* second) {
    *second = bcd_to_bin(read_cmos(CMOS_SECOND));
    *minute = bcd_to_bin(read_cmos(CMOS_MINUTE));
    *hour = bcd_to_bin(read_cmos(CMOS_HOUR));
}

// 读取当前日期
void get_current_date(unsigned char* day, unsigned char* month, unsigned char* year) {
    *day = bcd_to_bin(read_cmos(CMOS_DAY));
    *month = bcd_to_bin(read_cmos(CMOS_MONTH));
    *year = bcd_to_bin(read_cmos(CMOS_YEAR));
}

// 打印两位数（补零）
void print_two_digits(unsigned char num) {
    // 打印十位数
    if (num >= 10) {
        print_char('0' + (num / 10));
    } else {
        print_char('0');
    }
    // 打印个位数
    print_char('0' + (num % 10));
}

// 打印当前时间
void print_current_time() {
    unsigned char hour, minute, second;
    unsigned char day, month, year;
    
    get_current_time(&hour, &minute, &second);
    get_current_date(&day, &month, &year);
    
    // 打印日期
    print_two_digits(day);
    print_char('/');
    print_two_digits(month);
    print_char('/');
    print_two_digits(year);
    print_char(' ');
    
    // 打印时间
    print_two_digits(hour);
    print_char(':');
    print_two_digits(minute);
    print_char(':');
    print_two_digits(second);
}
