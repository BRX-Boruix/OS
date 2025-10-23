// Boruix OS TTY会话层实现
// 提供TTY会话管理和终端功能

#include "kernel/tty.h"
#include "drivers/display.h"
#include "../flanterm/flanterm.h"
#include "../flanterm/flanterm_backends/fb.h"
#include "../../kernel/shell/utils/string.h"

// TTY专用内存管理函数
extern void* tty_kmalloc(size_t size);
extern void tty_kfree(void* ptr);

// 全局内核TTY会话
tty_session_t *kernel_tty_session = NULL;

// 会话操作函数
static size_t session_write(void *session, const char *buf, size_t count) {
    tty_session_t *tty_session = (tty_session_t *)session;
    if (!tty_session || !tty_session->device) return 0;
    
    // 通过设备输出
    return tty_session->device->ops.write(tty_session->device->private_data, buf, count);
}

static size_t session_read(void *session, char *buf, size_t count) {
    tty_session_t *tty_session = (tty_session_t *)session;
    if (!tty_session || !tty_session->device) return 0;
    
    // 通过设备读取
    return tty_session->device->ops.read(tty_session->device->private_data, buf, count);
}

static void session_flush(void *session) {
    tty_session_t *tty_session = (tty_session_t *)session;
    if (!tty_session || !tty_session->device) return;
    
    // 通过设备刷新
    tty_session->device->ops.flush(tty_session->device->private_data);
}

static int session_ioctl(void *session, uint32_t cmd, uint32_t arg) {
    tty_session_t *tty_session = (tty_session_t *)session;
    if (!tty_session || !tty_session->device) return -1;
    
    // 通过设备ioctl
    return tty_session->device->ops.ioctl(tty_session->device->private_data, cmd, arg);
}

// 创建TTY会话
tty_session_t *tty_create_session(tty_device_t *device) {
    if (!device) return NULL;
    
    tty_session_t *session = (tty_session_t *)tty_kmalloc(sizeof(tty_session_t));
    if (!session) return NULL;
    
    // 初始化会话结构
    session->device = device;
    session->terminal = NULL;
    session->flags = 0;
    session->name = NULL;
    
    // 设置会话操作函数
    session->ops.write = session_write;
    session->ops.read = session_read;
    session->ops.flush = session_flush;
    session->ops.ioctl = session_ioctl;
    
    // 如果是图形设备，初始化flanterm终端
    if (device->type == TTY_DEVICE_GRAPHICS) {
        // 获取flanterm上下文（暂时设为NULL，使用默认显示系统）
        session->terminal = NULL;
    }
    
    return session;
}

// 销毁TTY会话
int tty_destroy_session(tty_session_t *session) {
    if (!session) return -1;
    
    // 清理会话资源
    if (session->name) {
        tty_kfree(session->name);
    }
    
    // 注意：不销毁设备，设备由设备管理器管理
    session->device = NULL;
    session->terminal = NULL;
    
    tty_kfree(session);
    return 0;
}

// 设置会话设备
int tty_set_session_device(tty_session_t *session, tty_device_t *device) {
    if (!session || !device) return -1;
    
    session->device = device;
    
    // 重新初始化终端上下文
    if (device->type == TTY_DEVICE_GRAPHICS) {
        session->terminal = NULL;  // 使用默认显示系统
    } else {
        session->terminal = NULL;
    }
    
    return 0;
}

// 初始化内核TTY会话
void tty_init_kernel_session(void) {
    if (kernel_tty_session) return;  // 已经初始化
    
    // 获取默认设备
    tty_device_t *default_device = tty_get_default_device();
    if (!default_device) {
        // 如果没有默认设备，尝试获取任何图形设备
        tty_device_t *current = tty_get_device("graphics");
        if (!current) {
            // 创建默认图形设备
            current = tty_alloc_device(TTY_DEVICE_GRAPHICS);
            if (current) {
                tty_register_device(current);
            }
        }
        default_device = current;
    }
    
    if (!default_device) return;
    
    // 创建内核会话
    kernel_tty_session = tty_create_session(default_device);
    if (kernel_tty_session) {
        kernel_tty_session->name = (char *)tty_kmalloc(16);
        if (kernel_tty_session->name) {
            shell_strcpy(kernel_tty_session->name, "kernel");
        }
    }
}

// 获取内核TTY会话
tty_session_t *tty_get_kernel_session(void) {
    return kernel_tty_session;
}

// 设置会话名称
int tty_set_session_name(tty_session_t *session, const char *name) {
    if (!session || !name) return -1;
    
    if (session->name) {
        tty_kfree(session->name);
    }
    
    session->name = (char *)tty_kmalloc(shell_strlen(name) + 1);
    if (session->name) {
        shell_strcpy(session->name, name);
        return 0;
    }
    
    return -1;
}

// 获取会话名称
const char *tty_get_session_name(tty_session_t *session) {
    if (!session) return NULL;
    return session->name;
}

// 设置会话标志
void tty_set_session_flags(tty_session_t *session, uint32_t flags) {
    if (session) {
        session->flags = flags;
    }
}

// 获取会话标志
uint32_t tty_get_session_flags(tty_session_t *session) {
    if (!session) return 0;
    return session->flags;
}

// 检查会话是否有效（在tty.c中实现）
