// Boruix OS Shell - lspci命令实现
// 列出所有PCI设备

#include "lspci.h"
#include "drivers/display.h"
#include "pci.h"  // PCI Zig驱动的C头文件

// 辅助函数：打印十六进制数（2位）
static void print_hex2(unsigned char value) {
    const char* hex_digits = "0123456789ABCDEF";
    char buf[3];
    buf[0] = hex_digits[(value >> 4) & 0xF];
    buf[1] = hex_digits[value & 0xF];
    buf[2] = '\0';
    print_string(buf);
}

// 辅助函数：打印十六进制数（4位）
static void print_hex4(unsigned short value) {
    const char* hex_digits = "0123456789ABCDEF";
    char buf[5];
    buf[0] = hex_digits[(value >> 12) & 0xF];
    buf[1] = hex_digits[(value >> 8) & 0xF];
    buf[2] = hex_digits[(value >> 4) & 0xF];
    buf[3] = hex_digits[value & 0xF];
    buf[4] = '\0';
    print_string(buf);
}

void cmd_lspci(int argc, char* argv[]) {
    (void)argc; 
    (void)argv;
    
    // 重新扫描PCI设备
    pci_init();
    
    size_t device_count = pci_get_device_count();
    
    if (device_count == 0) {
        print_string("No PCI devices found.\n");
        return;
    }
    
    print_string("PCI DEVICES\n");
    print_string("===========================================================\n");
    print_string("BUS:DEV.FN  VENDOR:DEVICE  CLASS  DESCRIPTION\n");
    print_string("-----------------------------------------------------------\n");
    
    // 遍历所有设备
    for (size_t i = 0; i < device_count; i++) {
        pci_device_t device;
        if (!pci_get_device(i, &device)) {
            continue;
        }
        
        // 格式: BB:DD.F  VVVV:DDDD  CC/SS  Description
        
        // 总线号
        print_hex2(device.bus);
        print_string(":");
        
        // 设备号
        print_hex2(device.device);
        print_string(".");
        
        // 功能号
        print_hex2(device.function);
        print_string("  ");
        
        // 厂商ID:设备ID
        print_hex4(device.vendor_id);
        print_string(":");
        print_hex4(device.device_id);
        print_string("  ");
        
        // 类别码/子类别
        print_hex2(device.class_code);
        print_string("/");
        print_hex2(device.subclass);
        print_string("  ");
        
        // 设备名称
        const char* class_name = pci_get_class_name(
            device.class_code, 
            device.subclass, 
            device.prog_if
        );
        print_string(class_name);
        print_string("\n");
    }
    
    print_string("-----------------------------------------------------------\n");
    print_string("Total devices: ");
    print_dec(device_count);
    print_string("\n");
}

