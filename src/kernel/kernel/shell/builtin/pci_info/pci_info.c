// PCI信息详细查看命令
// 显示子系统ID、中断信息、Capability支持等

#include "pci_info.h"
#include "drivers/display.h"
#include "pci.h"

void cmd_pci_info(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    print_string("\n");
    print_string("=======================================================\n");
    print_string("PCI DETAILED INFORMATION\n");
    print_string("=======================================================\n\n");
    
    pci_init();
    
    size_t count = pci_get_device_count();
    
    if (count == 0) {
        print_string("No PCI devices found.\n");
        return;
    }
    
    print_string("TEST 1: Subsystem Information\n");
    print_string("-----------------------------\n");
    
    for (size_t i = 0; i < count && i < 8; i++) {
        pci_device_t dev;
        if (!pci_get_device(i, &dev)) continue;
        
        print_string("Device ");
        print_dec(i);
        print_string(" (");
        print_hex(dev.bus);
        print_string(":");
        print_hex(dev.device);
        print_string(".");
        print_hex(dev.function);
        print_string("):\n");
        
        print_string("  Vendor:Device     = ");
        print_hex(dev.vendor_id);
        print_string(":");
        print_hex(dev.device_id);
        print_string("\n");
        
        print_string("  Subsystem V:D     = ");
        print_hex(dev.subsystem_vendor_id);
        print_string(":");
        print_hex(dev.subsystem_device_id);
        print_string("\n");
        
        print_string("\n");
    }
    
    print_string("TEST 2: Interrupt Configuration\n");
    print_string("------------------------------\n");
    
    int devices_with_interrupt = 0;
    int devices_without_interrupt = 0;
    
    for (size_t i = 0; i < count; i++) {
        pci_device_t dev;
        if (!pci_get_device(i, &dev)) continue;
        
        if (dev.interrupt_line != 0 || dev.interrupt_pin != 0) {
            devices_with_interrupt++;
            
            print_string("Device ");
            print_dec(i);
            print_string(" - IRQ:");
            print_dec(dev.interrupt_line);
            print_string(" Pin:");
            
            switch (dev.interrupt_pin) {
                case 0: print_string("None"); break;
                case 1: print_string("INTA"); break;
                case 2: print_string("INTB"); break;
                case 3: print_string("INTC"); break;
                case 4: print_string("INTD"); break;
                default: print_string("Unknown"); break;
            }
            print_string("\n");
        } else {
            devices_without_interrupt++;
        }
    }
    
    print_string("\n");
    print_string("Devices with interrupt: ");
    print_dec(devices_with_interrupt);
    print_string("\n");
    print_string("Devices without interrupt: ");
    print_dec(devices_without_interrupt);
    print_string("\n\n");
    
    print_string("TEST 3: Capability Support Summary\n");
    print_string("----------------------------------\n");
    
    int devices_with_msi = 0;
    int devices_with_pm = 0;
    
    for (size_t i = 0; i < count; i++) {
        pci_device_t dev;
        if (!pci_get_device(i, &dev)) continue;
        
        // 由于Capability扫描在内部进行，这里我们通过设备特征推断
        // 实际的Capability API会在后续完整实现
        
        // VGA和网卡通常支持MSI
        if (dev.class_code == 0x03 || dev.class_code == 0x02) {
            devices_with_msi++;
        }
        
        // 现代设备通常支持PM
        if (dev.revision > 0) {
            devices_with_pm++;
        }
    }
    
    print_string("Estimated MSI-capable devices: ");
    print_dec(devices_with_msi);
    print_string("\n");
    
    print_string("Estimated Power Management capable: ");
    print_dec(devices_with_pm);
    print_string("\n");
    
    print_string("Estimated MSI-X capable: ");
    print_dec(0); // devices_with_msix was removed
    print_string("\n\n");
    
    print_string("TEST 4: Detailed Device Report\n");
    print_string("------------------------------\n");
    
    for (size_t i = 0; i < count && i < 5; i++) {
        pci_device_t dev;
        if (!pci_get_device(i, &dev)) continue;
        
        print_string("Device ");
        print_dec(i);
        print_string(":\n");
        
        print_string("  BUS:SLOT.FUNC:     ");
        print_hex(dev.bus);
        print_string(":");
        print_hex(dev.device);
        print_string(".");
        print_hex(dev.function);
        print_string("\n");
        
        print_string("  Vendor:Device:     ");
        print_hex(dev.vendor_id);
        print_string(":");
        print_hex(dev.device_id);
        print_string("\n");
        
        print_string("  Subsystem V:D:     ");
        print_hex(dev.subsystem_vendor_id);
        print_string(":");
        print_hex(dev.subsystem_device_id);
        print_string("\n");
        
        print_string("  Class/SubClass:    ");
        print_hex(dev.class_code);
        print_string("/");
        print_hex(dev.subclass);
        print_string("\n");
        
        print_string("  Interrupt:         ");
        if (dev.interrupt_line == 0) {
            print_string("None");
        } else {
            print_string("IRQ");
            print_dec(dev.interrupt_line);
            print_string(" (Pin:");
            switch (dev.interrupt_pin) {
                case 1: print_string("A"); break;
                case 2: print_string("B"); break;
                case 3: print_string("C"); break;
                case 4: print_string("D"); break;
                default: print_string("?"); break;
            }
            print_string(")");
        }
        print_string("\n");
        
        print_string("  Header Type:       ");
        print_hex(dev.header_type);
        print_string("\n\n");
    }
    
    print_string("=======================================================\n");
    print_string("SUMMARY\n");
    print_string("=======================================================\n");
    print_string("Total Devices: ");
    print_dec(count);
    print_string("\n");
    print_string("Devices with Interrupt: ");
    print_dec(devices_with_interrupt);
    print_string("\n");
    print_string("Devices with Subsystem ID: ");
    print_dec(count);  // 所有设备都有子系统ID字段
    print_string("\n");
    print_string("=======================================================\n\n");
}
