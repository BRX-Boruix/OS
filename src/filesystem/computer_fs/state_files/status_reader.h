#ifndef STATUS_READER_H
#define STATUS_READER_H

#include <stdint.h>
#include <stdbool.h>

// 状态文件类型
typedef enum {
    STATUS_TYPE_CPU_FREQ = 1,      // CPU频率
    STATUS_TYPE_CPU_TEMP,          // CPU温度
    STATUS_TYPE_CPU_USAGE,         // CPU使用率
    STATUS_TYPE_MEM_TOTAL,         // 内存总量
    STATUS_TYPE_MEM_FREE,          // 可用内存
    STATUS_TYPE_MEM_USED,          // 已用内存
    STATUS_TYPE_DISK_SPACE,        // 磁盘空间
    STATUS_TYPE_NETWORK_SPEED,     // 网络速度
    STATUS_TYPE_BATTERY_LEVEL,     // 电池电量
    STATUS_TYPE_DISPLAY_BRIGHTNESS // 显示器亮度
} status_type_t;

// 状态数据结构
typedef struct status_data {
    status_type_t type;
    union {
        uint64_t uint_value;       // 无符号整数值
        int64_t int_value;         // 有符号整数值
        double float_value;        // 浮点数值
        char string_value[256];    // 字符串值
    } value;
    char unit[16];                 // 单位 (如 "MHz", "°C", "%")
    uint32_t timestamp;            // 时间戳
} status_data_t;

// 状态读取器
typedef struct status_reader {
    status_type_t type;
    char path[256];                // 文件路径
    int (*read_func)(status_data_t *data);  // 读取函数
    uint32_t update_interval;      // 更新间隔 (毫秒)
    uint32_t last_update;          // 上次更新时间
    status_data_t cached_data;     // 缓存的数据
} status_reader_t;

// 函数声明
int status_reader_init(void);
void status_reader_cleanup(void);
int status_register_reader(status_reader_t *reader);
int status_read_file(const char *path, char *buffer, size_t size);
int status_format_data(status_data_t *data, char *buffer, size_t size);

// 具体状态读取函数
int read_cpu_frequency(status_data_t *data);
int read_cpu_temperature(status_data_t *data);
int read_cpu_usage(status_data_t *data);
int read_memory_info(status_data_t *data);

#endif // STATUS_READER_H
