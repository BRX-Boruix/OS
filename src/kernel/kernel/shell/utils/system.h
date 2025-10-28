// Boruix OS Shell - 系统工具函数
// 提供系统级别的功能

#ifndef SYSTEM_H
#define SYSTEM_H

// 系统重启实现
void reboot_system(void);

// 系统关机实现 - 通用关机机制
void shutdown_system(void);

// 系统关机子机制
void acpi_shutdown(void);
void legacy_shutdown(void);

#endif // SYSTEM_H
