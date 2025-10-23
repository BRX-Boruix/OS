// Boruix OS TTY设备层实现
// 提供TTY设备抽象和管理

#include "kernel/tty.h"
#include "drivers/display.h"
#include "../../kernel/shell/utils/string.h"

// TTY专用内存管理函数现在在tty.h中定义为内联函数

// 设备链表头
static tty_device_t *tty_device_list = NULL;
static tty_device_t *default_device = NULL;

// 图形设备私有数据
typedef struct {
    void *framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
} graphics_private_t;

// 串口设备私有数据
typedef struct {
    uint16_t port;
    uint32_t baud_rate;
} serial_private_t;

// 图形设备操作函数
static size_t graphics_write(void *device, const char *buf, size_t count) {
    graphics_private_t *priv = (graphics_private_t *)device;
    if (!priv) return 0;
    
    // 使用现有的display系统
    for (size_t i = 0; i < count; i++) {
        print_char(buf[i]);
    }
    return count;
}

static size_t graphics_read(void *device, char *buf, size_t count) {
    // 图形设备不支持读取
    (void)device;
    (void)buf;
    (void)count;
    return 0;
}

static void graphics_flush(void *device) {
    graphics_private_t *priv = (graphics_private_t *)device;
    if (!priv) return;
    
    // 刷新显示
    display_flush();
}

static int graphics_ioctl(void *device, uint32_t cmd, uint32_t arg) {
    graphics_private_t *priv = (graphics_private_t *)device;
    if (!priv) return -1;
    
    switch (cmd) {
    case 0: // 获取分辨率
        *(uint32_t*)arg = priv->width;
        return 0;
    case 1: // 获取高度
        *(uint32_t*)arg = priv->height;
        return 0;
    default:
        return -1;
    }
}

// 串口设备操作函数（占位符）
static size_t serial_write(void *device, const char *buf, size_t count) {
    serial_private_t *priv = (serial_private_t *)device;
    if (!priv) return 0;
    
    // TODO: 实现串口输出
    (void)priv;
    (void)buf;
    (void)count;
    return count;
}

static size_t serial_read(void *device, char *buf, size_t count) {
    serial_private_t *priv = (serial_private_t *)device;
    if (!priv) return 0;
    
    // TODO: 实现串口输入
    (void)priv;
    (void)buf;
    (void)count;
    return 0;
}

static void serial_flush(void *device) {
    serial_private_t *priv = (serial_private_t *)device;
    if (!priv) return;
    
    // TODO: 实现串口刷新
    (void)priv;
}

static int serial_ioctl(void *device, uint32_t cmd, uint32_t arg) {
    serial_private_t *priv = (serial_private_t *)device;
    if (!priv) return -1;
    
    // TODO: 实现串口ioctl
    (void)priv;
    (void)cmd;
    (void)arg;
    return -1;
}

// TTY设备管理函数
tty_device_t *tty_alloc_device(tty_device_type_t type) {
    print_string("[TTY] Allocating device, type: ");
    if (type == TTY_DEVICE_GRAPHICS) {
        print_string("GRAPHICS");
    } else if (type == TTY_DEVICE_SERIAL) {
        print_string("SERIAL");
    } else {
        print_string("UNKNOWN");
    }
    print_string("\n");
    
    tty_device_t *device = (tty_device_t *)tty_kmalloc(sizeof(tty_device_t));
    if (!device) {
        print_string("[TTY] Failed to allocate device structure\n");
        return NULL;
    }
    print_string("[TTY] Device structure allocated\n");
    
    // 初始化设备结构
    device->type = type;
    device->name[0] = '\0';
    device->private_data = NULL;
    device->next = NULL;
    
    // 根据设备类型设置操作函数和私有数据
    switch (type) {
    case TTY_DEVICE_GRAPHICS: {
        print_string("[TTY] Allocating graphics private data\n");
        graphics_private_t *priv = (graphics_private_t *)tty_kmalloc(sizeof(graphics_private_t));
        if (!priv) {
            print_string("[TTY] Failed to allocate graphics private data\n");
            tty_kfree(device);
            return NULL;
        }
        print_string("[TTY] Graphics private data allocated\n");
        
        // 初始化图形设备私有数据
        priv->framebuffer = NULL;  // 将在注册时设置
        priv->width = 0;
        priv->height = 0;
        priv->pitch = 0;
        priv->bpp = 32;
        
        device->private_data = priv;
        device->ops.write = graphics_write;
        device->ops.read = graphics_read;
        device->ops.flush = graphics_flush;
        device->ops.ioctl = graphics_ioctl;
        
        shell_strcpy(device->name, "graphics");
        break;
    }
    
    case TTY_DEVICE_SERIAL: {
        serial_private_t *priv = (serial_private_t *)tty_kmalloc(sizeof(serial_private_t));
        if (!priv) {
            tty_kfree(device);
            return NULL;
        }
        
        // 初始化串口设备私有数据
        priv->port = 0x3F8;  // COM1
        priv->baud_rate = 115200;
        
        device->private_data = priv;
        device->ops.write = serial_write;
        device->ops.read = serial_read;
        device->ops.flush = serial_flush;
        device->ops.ioctl = serial_ioctl;
        
        shell_strcpy(device->name, "serial");
        break;
    }
    
    case TTY_DEVICE_VGA:
        // VGA设备使用图形设备的操作函数
        device->private_data = NULL;
        device->ops.write = graphics_write;
        device->ops.read = graphics_read;
        device->ops.flush = graphics_flush;
        device->ops.ioctl = graphics_ioctl;
        
        shell_strcpy(device->name, "vga");
        break;
    }
    
    return device;
}

int tty_register_device(tty_device_t *device) {
    if (!device) return -1;
    
    // 添加到设备链表
    device->next = tty_device_list;
    tty_device_list = device;
    
    // 如果是第一个图形设备，设为默认设备
    if (!default_device && device->type == TTY_DEVICE_GRAPHICS) {
        default_device = device;
    }
    
    return 0;
}

int tty_unregister_device(tty_device_t *device) {
    if (!device) return -1;
    
    // 从设备链表中移除
    if (tty_device_list == device) {
        tty_device_list = device->next;
    } else {
        tty_device_t *current = tty_device_list;
        while (current && current->next != device) {
            current = current->next;
        }
        if (current) {
            current->next = device->next;
        }
    }
    
    // 如果移除的是默认设备，重新选择默认设备
    if (default_device == device) {
        default_device = tty_device_list;
        while (default_device && default_device->type != TTY_DEVICE_GRAPHICS) {
            default_device = default_device->next;
        }
    }
    
    return 0;
}

tty_device_t *tty_get_device(const char *name) {
    if (!name) return NULL;
    
    tty_device_t *current = tty_device_list;
    while (current) {
        if (shell_strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

tty_device_t *tty_get_default_device(void) {
    return default_device;
}

// 初始化TTY设备系统
void tty_init_devices(void) {
    // 创建图形设备
    tty_device_t *graphics_device = tty_alloc_device(TTY_DEVICE_GRAPHICS);
    if (graphics_device) {
        print_string("[TTY] Graphics device allocated\n");
        tty_register_device(graphics_device);
        print_string("[TTY] Graphics device registered\n");
    } else {
        print_string("[TTY] Failed to allocate graphics device\n");
    }
    
    // 创建串口设备（可选）
    tty_device_t *serial_device = tty_alloc_device(TTY_DEVICE_SERIAL);
    if (serial_device) {
        print_string("[TTY] Serial device allocated\n");
        tty_register_device(serial_device);
        print_string("[TTY] Serial device registered\n");
    } else {
        print_string("[TTY] Failed to allocate serial device\n");
    }
}
