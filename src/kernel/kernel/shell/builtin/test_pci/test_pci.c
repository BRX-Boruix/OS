// PCI驱动测试程序
// 验证BAR大小计算、Segment支持等功能

#include "test_pci.h"
#include "drivers/display.h"
#include "pci.h"

void cmd_test_pci(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    print_string("\n");
    print_string("=======================================================\n");
    print_string("PCI DRIVER V2 - COMPREHENSIVE TEST\n");
    print_string("=======================================================\n\n");
    
    // 测试1: 基础信息
    print_string("TEST 1: Basic Information\n");
    print_string("-------------------------\n");
    
    pci_init();
    
    size_t count = pci_get_device_count();
    uint32_t segments = pci_get_segment_count();
    uint8_t mode = pci_get_mode();
    
    print_string("PCI Mode: ");
    print_string(mode == 0 ? "Legacy I/O" : "MCFG");
    print_string("\n");
    
    print_string("Segments: ");
    print_dec(segments);
    print_string("\n");
    
    print_string("Devices Found: ");
    print_dec(count);
    print_string("\n\n");
    
    // 测试2: BAR大小验证
    print_string("TEST 2: BAR Size Calculation\n");
    print_string("---------------------------\n");
    
    int valid_bars = 0;
    int zero_size_bars = 0;
    
    for (size_t i = 0; i < count && i < 5; i++) {
        pci_device_t dev;
        if (!pci_get_device(i, &dev)) continue;
        
        print_string("Device ");
        print_dec(i);
        print_string(": ");
        print_hex(dev.vendor_id);
        print_string(":");
        print_hex(dev.device_id);
        print_string(" - BARs:\n");
        
        for (uint8_t j = 0; j < 6; j++) {
            uint64_t addr, size;
            if (pci_get_bar(i, j, &addr, &size)) {
                valid_bars++;
                
                print_string("  BAR");
                print_dec(j);
                print_string(": addr=");
                print_hex(addr);
                print_string(" size=");
                print_hex(size);
                
                if (size == 0) {
                    print_string(" [ZERO SIZE]");
                    zero_size_bars++;
                }
                print_string("\n");
            }
        }
        print_string("\n");
    }
    
    print_string("Valid BARs: ");
    print_dec(valid_bars);
    print_string("\n");
    print_string("Zero-size BARs: ");
    print_dec(zero_size_bars);
    print_string("\n\n");
    
    // 测试3: 完整BAR信息
    print_string("TEST 3: BAR Info Structure\n");
    print_string("-------------------------\n");
    
    if (count > 0) {
        pci_device_t dev;
        pci_get_device(0, &dev);
        
        for (uint8_t j = 0; j < 2; j++) {
            pci_bar_info_t bar_info;
            if (pci_get_bar_info(0, j, &bar_info)) {
                print_string("Device 0, BAR");
                print_dec(j);
                print_string(":\n");
                
                print_string("  Address: ");
                print_hex(bar_info.address);
                print_string("\n");
                
                print_string("  Size: ");
                print_hex(bar_info.size);
                print_string("\n");
                
                print_string("  Type: ");
                switch (bar_info.type) {
                    case 0: print_string("Memory 32-bit"); break;
                    case 2: print_string("Memory 64-bit"); break;
                    case 3: print_string("IO"); break;
                    default: print_string("Unknown"); break;
                }
                print_string("\n");
                
                print_string("  Prefetchable: ");
                print_string(bar_info.prefetchable ? "Yes" : "No");
                print_string("\n\n");
            }
        }
    }
    
    // 测试4: 多设备检查
    print_string("TEST 4: Multi-Device Summary\n");
    print_string("----------------------------\n");
    
    for (size_t i = 0; i < count; i++) {
        pci_device_t dev;
        if (!pci_get_device(i, &dev)) continue;
        
        print_string(dev.bus < 10 ? "0" : "");
        print_dec(dev.bus);
        print_string(":");
        print_string(dev.device < 10 ? "0" : "");
        print_dec(dev.device);
        print_string(".");
        print_dec(dev.function);
        print_string(" - ");
        
        const char* class_name = pci_get_class_name(
            dev.class_code, dev.subclass, dev.prog_if
        );
        print_string(class_name);
        print_string("\n");
    }
    
    print_string("Total Tests: 4 | ");
    print_string("Devices Scanned: ");
    print_dec(count);
    print_string(" | ");
    print_string("BARs Analyzed: ");
    print_dec(valid_bars);
    print_string(" | ");
    
    if (zero_size_bars == 0 && valid_bars > 0) {
        print_string("Result: PASS");
    } else if (zero_size_bars > 0) {
        print_string("Result: PARTIAL");
    } else {
        print_string("Result: INFO");
    }
    
    print_string("\n");
}
