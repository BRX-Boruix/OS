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

// 外部函数声明：获取HHDM偏移量（由Rust内存管理器提供）
extern uint64_t rust_get_hhdm_offset(void);

// PCI设备信息结构（与Zig中的PCIDeviceC对应）
typedef struct {
    uint8_t  bus;                    // 总线号
    uint8_t  device;                 // 设备号
    uint8_t  function;               // 功能号
    uint16_t vendor_id;              // 厂商ID
    uint16_t device_id;              // 设备ID
    uint8_t  class_code;             // 类别码
    uint8_t  subclass;               // 子类别
    uint8_t  prog_if;                // 编程接口
    uint8_t  revision;               // 版本ID
    uint8_t  header_type;            // 头部类型
    uint16_t subsystem_vendor_id;    // 子系统厂商ID（新增）
    uint16_t subsystem_device_id;    // 子系统设备ID（新增）
    uint8_t  interrupt_line;         // 中断号（新增）
    uint8_t  interrupt_pin;          // 中断脚（新增）
} pci_device_t;

// BAR类型枚举
typedef enum {
    PCI_BAR_IO = 3,          // IO空间
    PCI_BAR_MEM32 = 0,       // 32位内存
    PCI_BAR_MEM64 = 2,       // 64位内存
} pci_bar_type_t;

// BAR信息结构
typedef struct {
    uint64_t address;        // BAR基地址
    uint64_t size;           // BAR大小
    uint8_t  type;           // BAR类型 (见 pci_bar_type_t)
    uint8_t  prefetchable;   // 是否可预取（仅内存BAR）
} pci_bar_info_t;

// 初始化PCI驱动并扫描所有设备
void pci_init(void);

// 获取扫描到的设备数量
size_t pci_get_device_count(void);

// 获取PCI Segment数量（支持多Segment系统）
uint32_t pci_get_segment_count(void);

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

// 获取PCI模式（0=Legacy, 1=MCFG）
uint8_t pci_get_mode(void);

// 获取设备BAR信息（包含大小）
bool pci_get_bar(size_t index, uint8_t bar_idx, uint64_t* out_addr, uint64_t* out_size);

// 获取设备完整BAR信息（包括类型和标志）
// 返回: true表示该BAR有效，false表示无效或超出范围
bool pci_get_bar_info(size_t device_index, uint8_t bar_index, pci_bar_info_t* out_bar);

// 获取设备的子系统信息（新增）
// 返回: true表示成功，false表示索引无效
bool pci_get_subsystem_info(size_t device_index, uint16_t* out_vendor_id, uint16_t* out_device_id);

// 获取设备的中断信息（新增）
// 返回: true表示成功，false表示索引无效
// out_line: 中断号（0=无中断）
// out_pin: 中断脚（1=INTA, 2=INTB, 3=INTC, 4=INTD, 0=无）
bool pci_get_interrupt_info(size_t device_index, uint8_t* out_interrupt_line, uint8_t* out_interrupt_pin);

// ============================================================================
// 设备过滤和查询API（新增）
// ============================================================================

// 按Vendor ID查找设备
// 返回: 找到的设备数量
// out_indices: 指向结果数组的指针，用于存储设备索引
// max_count: 最多返回的设备数量
size_t pci_find_by_vendor(uint16_t vendor_id, size_t* out_indices, size_t max_count);

// 按Device ID查找设备
// 返回: 找到的设备数量
size_t pci_find_by_device(uint16_t device_id, size_t* out_indices, size_t max_count);

// 按Vendor和Device ID同时查找（精确匹配）
// 返回: 找到的设备数量
size_t pci_find_by_vendor_and_device(uint16_t vendor_id, uint16_t device_id, size_t* out_indices, size_t max_count);

// 按Class Code查找设备
// 返回: 找到的设备数量
size_t pci_find_by_class(uint8_t class_code, size_t* out_indices, size_t max_count);

// 按Class和SubClass查找设备
// 返回: 找到的设备数量
size_t pci_find_by_class_and_subclass(uint8_t class_code, uint8_t subclass, size_t* out_indices, size_t max_count);

// 按Bus号查找设备
// 返回: 找到的设备数量
size_t pci_find_by_bus(uint8_t bus, size_t* out_indices, size_t max_count);

#ifdef __cplusplus
}
#endif

#endif // PCI_ZIG_H

