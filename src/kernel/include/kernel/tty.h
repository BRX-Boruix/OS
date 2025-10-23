// Boruix OS TTY系统头文件
// 参考CoolPotOS实现的现代TTY架构

#ifndef BORUIX_TTY_H
#define BORUIX_TTY_H

#include "kernel/types.h"
#include "stdarg.h"

// TTY设备类型
typedef enum {
    TTY_DEVICE_GRAPHICS = 0,  // 图形设备（framebuffer）
    TTY_DEVICE_SERIAL = 1,    // 串口设备
    TTY_DEVICE_VGA = 2        // VGA文本模式（兼容）
} tty_device_type_t;

// TTY设备操作接口
typedef struct tty_device_ops {
    size_t (*write)(void *device, const char *buf, size_t count);
    size_t (*read)(void *device, char *buf, size_t count);
    void (*flush)(void *device);
    int (*ioctl)(void *device, uint32_t cmd, uint32_t arg);
} tty_device_ops_t;

// TTY设备结构
typedef struct tty_device {
    tty_device_type_t type;
    char name[32];
    void *private_data;        // 设备私有数据
    tty_device_ops_t ops;     // 设备操作函数
    struct tty_device *next;   // 链表节点
} tty_device_t;

// TTY会话操作接口
typedef struct tty_session_ops {
    size_t (*write)(void *session, const char *buf, size_t count);
    size_t (*read)(void *session, char *buf, size_t count);
    void (*flush)(void *session);
    int (*ioctl)(void *session, uint32_t cmd, uint32_t arg);
} tty_session_ops_t;

// TTY会话结构
typedef struct tty_session {
    void *terminal;                    // 终端上下文（flanterm）
    tty_device_t *device;             // 关联的设备
    tty_session_ops_t ops;            // 会话操作函数
    uint32_t flags;                   // 会话标志
    char *name;                       // 会话名称
} tty_session_t;

// 内核日志级别
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4
} log_level_t;

// 颜色定义（兼容现有系统）
#define TTY_COLOR_BLACK    0
#define TTY_COLOR_RED      1
#define TTY_COLOR_GREEN    2
#define TTY_COLOR_YELLOW   3
#define TTY_COLOR_BLUE     4
#define TTY_COLOR_MAGENTA  5
#define TTY_COLOR_CYAN     6
#define TTY_COLOR_WHITE    7

// 内核日志缓冲区大小
#define KMSG_BUFFER_SIZE (1 << 17)  // 128KB

// 前向声明
struct tty_device;
struct tty_session;

// 全局TTY会话（内核使用）
extern tty_session_t *kernel_tty_session;

// TTY设备管理
tty_device_t *tty_alloc_device(tty_device_type_t type);
int tty_register_device(tty_device_t *device);
int tty_unregister_device(tty_device_t *device);
tty_device_t *tty_get_device(const char *name);
tty_device_t *tty_get_default_device(void);
void tty_init_devices(void);

// TTY会话管理
tty_session_t *tty_create_session(tty_device_t *device);
int tty_destroy_session(tty_session_t *session);
int tty_set_session_device(tty_session_t *session, tty_device_t *device);
void tty_init_kernel_session(void);
tty_session_t *tty_get_kernel_session(void);
int tty_set_session_name(tty_session_t *session, const char *name);
const char *tty_get_session_name(tty_session_t *session);
void tty_set_session_flags(tty_session_t *session, uint32_t flags);
uint32_t tty_get_session_flags(tty_session_t *session);
bool tty_is_session_valid(tty_session_t *session);

// TTY初始化
void tty_init(void);

// 内核输出接口
void kprint(const char *str);
void kprintf(const char *fmt, ...);
void kprint_color(uint8_t fg, uint8_t bg, const char *fmt, ...);

// 内核日志系统
void klog_init(void);
void klog_putc(char c);
void klog_write(const char *str);
int klog_getc(void);
void klog_flush(void);

// 日志级别输出
void kdebug(const char *fmt, ...);
void kinfo(const char *fmt, ...);
void kwarn(const char *fmt, ...);
void kerror(const char *fmt, ...);
void kfatal(const char *fmt, ...);

// 系统状态检查
bool tty_is_initialized(void);
void tty_cleanup(void);
void tty_test(void);
int tty_switch_device(const char *device_name);
const char *tty_get_current_device_name(void);
void tty_list_devices(void);

// TTY内存管理函数
extern void* tty_kmalloc(size_t size);
extern void tty_kfree(void* ptr);
extern void tty_memory_stats(size_t *total, size_t *used, size_t *free, size_t *peak);
extern void* tty_kmalloc_large(size_t size);
extern void tty_kfree_large(void* ptr);
extern void* tty_map_page(uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags);
extern uint64_t tty_get_physical_addr(uint64_t virtual_addr);

#endif // BORUIX_TTY_H