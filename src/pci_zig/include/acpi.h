// ACPI基础定义 - 用于PCI MCFG支持
// 最小化ACPI实现，仅包含PCI所需部分

#ifndef PCI_ACPI_H
#define PCI_ACPI_H

#include <stdint.h>

// ACPI标准描述符头
typedef struct {
    char     signature[4];      // 表签名
    uint32_t length;            // 表长度
    uint8_t  revision;          // 版本
    uint8_t  checksum;          // 校验和
    char     oem_id[6];         // OEM ID
    char     oem_table_id[8];   // OEM表ID
    uint32_t oem_revision;      // OEM版本
    uint32_t creator_id;        // 创建者ID
    uint32_t creator_revision;  // 创建者版本
} __attribute__((packed)) acpi_sdt_header_t;

// RSDP结构（根系统描述指针）
typedef struct {
    char     signature[8];      // "RSD PTR "
    uint8_t  checksum;          // 校验和
    char     oem_id[6];         // OEM ID
    uint8_t  revision;          // ACPI版本（0=1.0, 2=2.0+）
    uint32_t rsdt_address;      // RSDT物理地址
    // ACPI 2.0+扩展字段
    uint32_t length;            // 结构长度
    uint64_t xsdt_address;      // XSDT物理地址
    uint8_t  extended_checksum; // 扩展校验和
    uint8_t  reserved[3];       // 保留
} __attribute__((packed)) rsdp_t;

// RSDT（根系统描述表）
typedef struct {
    acpi_sdt_header_t header;
    uint32_t          entries[];  // 指向其他表的指针数组
} __attribute__((packed)) rsdt_t;

// XSDT（扩展系统描述表）
typedef struct {
    acpi_sdt_header_t header;
    uint64_t          entries[];  // 指向其他表的指针数组
} __attribute__((packed)) xsdt_t;

// MCFG条目（PCI配置空间基地址）
typedef struct {
    uint64_t base_addr;   // PCI配置空间基地址
    uint16_t segment;     // PCI段组号
    uint8_t  start_bus;   // 起始总线号
    uint8_t  end_bus;     // 结束总线号
    uint32_t reserved;    // 保留
} __attribute__((packed)) mcfg_entry_t;

// MCFG表（PCI Express内存映射配置）
typedef struct {
    acpi_sdt_header_t header;
    uint8_t           reserved[8];
    mcfg_entry_t      entries[];  // 动态长度
} __attribute__((packed)) mcfg_t;

// MCFG信息（运行时状态）
typedef struct {
    mcfg_t  *mcfg;      // MCFG表指针
    size_t   count;     // 条目数量
    int      enabled;   // 是否启用
} mcfg_info_t;

#endif // PCI_ACPI_H

