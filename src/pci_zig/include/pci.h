/*
 * PCI驱动 - C头文件
 * Zig实现的C接口
 */

#ifndef PCI_ZIG_H
#define PCI_ZIG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// PCI设备信息结构（与Zig中的PCIDeviceC对应）
typedef struct {
    uint8_t  bus;          // 总线号
    uint8_t  device;       // 设备号
    uint8_t  function;     // 功能号
    uint16_t vendor_id;    // 厂商ID
    uint16_t device_id;    // 设备ID
    uint8_t  class_code;   // 类别码
    uint8_t  subclass;     // 子类别
    uint8_t  prog_if;      // 编程接口
    uint8_t  revision;     // 版本ID
    uint8_t  header_type;  // 头部类型
} pci_device_t;

// 初始化PCI驱动并扫描所有设备
void pci_init(void);

// 获取扫描到的设备数量
size_t pci_get_device_count(void);

// 获取指定索引的设备信息
// 返回: true表示成功，false表示索引无效
bool pci_get_device(size_t index, pci_device_t *out_device);

// 获取设备类别名称
// 返回: 设备类别名称字符串（静态字符串，不需要释放）
const char* pci_get_class_name(uint8_t class_code, uint8_t subclass, uint8_t prog_if);

// 读取PCI配置空间（16位）
uint16_t pci_read_config_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

// 读取PCI配置空间（32位）
uint32_t pci_read_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

// 写入PCI配置空间（32位）
void pci_write_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif // PCI_ZIG_H

