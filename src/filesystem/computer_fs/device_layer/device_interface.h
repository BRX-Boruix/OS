#ifndef DEVICE_INTERFACE_H
#define DEVICE_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

// 设备类型定义
typedef enum {
    DEVICE_TYPE_CPU = 1,
    DEVICE_TYPE_MEMORY,
    DEVICE_TYPE_STORAGE,
    DEVICE_TYPE_NETWORK,
    DEVICE_TYPE_DISPLAY,
    DEVICE_TYPE_AUDIO,
    DEVICE_TYPE_INPUT,
    DEVICE_TYPE_POWER,
    DEVICE_TYPE_UNKNOWN
} device_type_t;

// 设备状态
typedef enum {
    DEVICE_STATUS_ACTIVE = 1,
    DEVICE_STATUS_INACTIVE,
    DEVICE_STATUS_ERROR,
    DEVICE_STATUS_UNKNOWN
} device_status_t;

// 设备操作接口
typedef struct device_ops {
    int (*init)(void *device);
    int (*cleanup)(void *device);
    int (*read_status)(void *device, char *buffer, size_t size);
    int (*write_control)(void *device, const char *buffer, size_t size);
    int (*get_info)(void *device, char *buffer, size_t size);
} device_ops_t;

// 设备描述符
typedef struct device_desc {
    uint32_t device_id;            // 设备ID
    device_type_t type;            // 设备类型
    char name[64];                 // 设备名称
    char path[256];                // 在 computer:/ 中的路径
    device_status_t status;        // 设备状态
    
    device_ops_t *ops;             // 设备操作接口
    void *private_data;            // 设备私有数据
    
    struct device_desc *next;      // 链表指针
} device_desc_t;

// 设备管理器
typedef struct device_manager {
    device_desc_t *device_list;    // 设备链表
    uint32_t next_device_id;       // 下一个设备ID
    int device_count;              // 设备数量
} device_manager_t;

// 函数声明
int device_manager_init(void);
void device_manager_cleanup(void);
int device_register(device_desc_t *device);
int device_unregister(uint32_t device_id);
device_desc_t* device_find_by_id(uint32_t device_id);
device_desc_t* device_find_by_path(const char *path);
int device_scan_hardware(void);

#endif // DEVICE_INTERFACE_H
