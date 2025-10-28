// PCI驱动 - Zig实现
// 参考Uinxed-Kernel和CoolPotOS的实现

const std = @import("std");

// ============================================================================
// 常量定义
// ============================================================================

pub const PCI_COMMAND_PORT: u16 = 0xCF8;
pub const PCI_DATA_PORT: u16 = 0xCFC;

pub const PCI_CONF_VENDOR: u8 = 0x00;
pub const PCI_CONF_DEVICE: u8 = 0x02;
pub const PCI_CONF_REVISION: u8 = 0x08;
pub const PCI_CONF_HEADER_TYPE: u8 = 0x0E;

pub const MAX_BUS: u16 = 256;
pub const MAX_DEVICE: u8 = 32;
pub const MAX_FUNCTION: u8 = 8;

// ============================================================================
// 类型定义
// ============================================================================

pub const PCIAddress = packed struct {
    function: u3,
    device: u5,
    bus: u8,

    pub fn init(bus: u8, device: u5, function: u3) PCIAddress {
        return .{ .bus = bus, .device = device, .function = function };
    }
};

pub const PCIDevice = struct {
    address: PCIAddress,
    vendor_id: u16,
    device_id: u16,
    class_code: u8,
    subclass: u8,
    prog_if: u8,
    revision: u8,
    header_type: u8,
};

pub const DeviceClass = struct {
    code: u32,
    name: []const u8,
};

// ============================================================================
// 串口调试输出
// ============================================================================

const COM1: u16 = 0x3F8;

inline fn outb(port: u16, value: u8) void {
    asm volatile ("outb %[value], %[port]"
        :
        : [value] "{al}" (value),
          [port] "{dx}" (port),
    );
}

fn serial_putc(c: u8) void {
    outb(COM1, c);
}

fn serial_print(s: []const u8) void {
    for (s) |c| {
        serial_putc(c);
    }
}

fn serial_print_hex(value: u32) void {
    const hex_chars = "0123456789ABCDEF";
    var buf: [10]u8 = undefined;
    buf[0] = '0';
    buf[1] = 'x';
    var i: usize = 9;
    var v = value;
    while (i >= 2) : (i -= 1) {
        buf[i] = hex_chars[v & 0xF];
        v >>= 4;
        if (i == 2) break;
    }
    serial_print(&buf);
}

// ============================================================================
// IO端口操作
// ============================================================================

inline fn outl(port: u16, value: u32) void {
    asm volatile ("outl %[value], %[port]"
        :
        : [value] "{eax}" (value),
          [port] "{dx}" (port),
    );
}

inline fn inl(port: u16) u32 {
    return asm volatile ("inl %[port], %[result]"
        : [result] "={eax}" (-> u32),
        : [port] "{dx}" (port),
    );
}

// ============================================================================
// PCI配置空间访问
// ============================================================================

pub fn pciConfigReadDword(addr: PCIAddress, offset: u8) u32 {
    const config_addr: u32 =
        (@as(u32, 1) << 31) |
        (@as(u32, addr.bus) << 16) |
        (@as(u32, addr.device) << 11) |
        (@as(u32, addr.function) << 8) |
        (@as(u32, offset & 0xFC));
    outl(PCI_COMMAND_PORT, config_addr);
    return inl(PCI_DATA_PORT);
}

pub fn pciConfigWriteDword(addr: PCIAddress, offset: u8, value: u32) void {
    const config_addr: u32 =
        (@as(u32, 1) << 31) |
        (@as(u32, addr.bus) << 16) |
        (@as(u32, addr.device) << 11) |
        (@as(u32, addr.function) << 8) |
        (@as(u32, offset & 0xFC));
    outl(PCI_COMMAND_PORT, config_addr);
    outl(PCI_DATA_PORT, value);
}

pub fn pciConfigReadByte(addr: PCIAddress, offset: u8) u8 {
    const dword = pciConfigReadDword(addr, offset & 0xFC);
    const shift = @as(u5, @intCast((offset & 0x03) * 8));
    return @as(u8, @truncate(dword >> shift));
}

pub fn pciConfigReadWord(addr: PCIAddress, offset: u8) u16 {
    const dword = pciConfigReadDword(addr, offset & 0xFC);
    const shift = @as(u5, @intCast((offset & 0x02) * 8));
    return @as(u16, @truncate(dword >> shift));
}

// ============================================================================
// PCI设备检测和扫描
// ============================================================================

pub fn pciDeviceExists(addr: PCIAddress) bool {
    const vendor_id = pciConfigReadWord(addr, PCI_CONF_VENDOR);
    return vendor_id != 0xFFFF;
}

pub fn pciReadDevice(addr: PCIAddress) PCIDevice {
    const vendor_device = pciConfigReadDword(addr, PCI_CONF_VENDOR);
    const vendor_id = @as(u16, @truncate(vendor_device));
    const device_id = @as(u16, @truncate(vendor_device >> 16));

    const class_info = pciConfigReadDword(addr, PCI_CONF_REVISION);
    const revision = @as(u8, @truncate(class_info));
    const prog_if = @as(u8, @truncate(class_info >> 8));
    const subclass = @as(u8, @truncate(class_info >> 16));
    const class_code = @as(u8, @truncate(class_info >> 24));

    const header_type = pciConfigReadByte(addr, PCI_CONF_HEADER_TYPE);

    return PCIDevice{
        .address = addr,
        .vendor_id = vendor_id,
        .device_id = device_id,
        .class_code = class_code,
        .subclass = subclass,
        .prog_if = prog_if,
        .revision = revision,
        .header_type = header_type & 0x7F,
    };
}

pub fn pciIsMultifunction(addr: PCIAddress) bool {
    const header_type = pciConfigReadByte(addr, PCI_CONF_HEADER_TYPE);
    return (header_type & 0x80) != 0;
}

// ============================================================================
// 设备类别名称映射
// ============================================================================

const device_classes = [_]DeviceClass{
    .{ .code = 0x000000, .name = "Non-VGA Unclassified Device" },
    .{ .code = 0x000100, .name = "VGA-Compatible Unclassified Device" },
    .{ .code = 0x010000, .name = "SCSI Bus Controller" },
    .{ .code = 0x010100, .name = "IDE Controller" },
    .{ .code = 0x010200, .name = "Floppy Disk Controller" },
    .{ .code = 0x010600, .name = "SATA Controller" },
    .{ .code = 0x010800, .name = "NVMe Controller" },
    .{ .code = 0x020000, .name = "Ethernet Controller" },
    .{ .code = 0x030000, .name = "VGA Controller" },
    .{ .code = 0x030200, .name = "3D Controller" },
    .{ .code = 0x040000, .name = "Multimedia Video Controller" },
    .{ .code = 0x040100, .name = "Multimedia Audio Controller" },
    .{ .code = 0x040300, .name = "Audio Device" },
    .{ .code = 0x050000, .name = "RAM Controller" },
    .{ .code = 0x060000, .name = "Host Bridge" },
    .{ .code = 0x060100, .name = "ISA Bridge" },
    .{ .code = 0x060400, .name = "PCI-to-PCI Bridge" },
    .{ .code = 0x070000, .name = "Serial Controller" },
    .{ .code = 0x0c0300, .name = "USB Controller" },
    .{ .code = 0x0c0500, .name = "SMBus Controller" },
};

pub fn getDeviceClassName(class_code: u8, subclass: u8, prog_if: u8) []const u8 {
    const full_code = (@as(u32, class_code) << 16) |
        (@as(u32, subclass) << 8) |
        @as(u32, prog_if);

    for (device_classes) |dc| {
        if (dc.code == full_code) {
            return dc.name;
        }
    }

    const partial_code = full_code & 0xFFFF00;
    for (device_classes) |dc| {
        if (dc.code == partial_code) {
            return dc.name;
        }
    }

    return "Unknown Device";
}

// ============================================================================
// C FFI导出接口
// ============================================================================

var device_list: [256]PCIDevice = undefined;
var device_count: usize = 0;

pub const PCIDeviceC = extern struct {
    bus: u8,
    device: u8,
    function: u8,
    vendor_id: u16,
    device_id: u16,
    class_code: u8,
    subclass: u8,
    prog_if: u8,
    revision: u8,
    header_type: u8,
};

export fn pci_init() void {
    serial_print("[PCI] Starting scan...\n");
    device_count = 0;
    var bus: u8 = 0;
    while (bus < 16) : (bus += 1) {
        serial_print("[PCI] Scanning bus ");
        serial_print_hex(bus);
        serial_print("\n");
        
        var dev: u8 = 0;
        while (dev < MAX_DEVICE) : (dev += 1) {
            const addr = PCIAddress.init(bus, @intCast(dev), 0);
            
            // 检查function 0是否存在（必须先检查这个）
            if (!pciDeviceExists(addr)) continue;

            serial_print("[PCI] Found device at ");
            serial_print_hex(bus);
            serial_print(":");
            serial_print_hex(dev);
            serial_print(":0\n");

            const device = pciReadDevice(addr);
            if (device_count < device_list.len) {
                device_list[device_count] = device;
                device_count += 1;
            }

            // 只有function 0存在且是多功能设备时，才扫描其他功能
            const is_multifunction = pciIsMultifunction(addr);
            if (is_multifunction) {
                serial_print("[PCI] Multifunction device, scanning functions\n");
                var func: u8 = 1;
                while (func < MAX_FUNCTION) : (func += 1) {
                    const func_addr = PCIAddress.init(bus, @intCast(dev), @intCast(func & 0x07));
                    
                    // 检查这个功能是否真的存在
                    const vendor = pciConfigReadWord(func_addr, PCI_CONF_VENDOR);
                    if (vendor == 0xFFFF or vendor == 0x0000) continue;
                    
                    serial_print("[PCI] Found function ");
                    serial_print_hex(func);
                    serial_print("\n");
                    
                    const func_device = pciReadDevice(func_addr);
                    if (device_count < device_list.len) {
                        device_list[device_count] = func_device;
                        device_count += 1;
                    }
                }
            }
        }
    }
    serial_print("[PCI] Scan complete, found ");
    serial_print_hex(@intCast(device_count));
    serial_print(" devices\n");
}

export fn pci_get_device_count() usize {
    return device_count;
}

export fn pci_get_device(index: usize, out_device: *PCIDeviceC) bool {
    if (index >= device_count) return false;

    const dev = &device_list[index];
    out_device.bus = dev.address.bus;
    out_device.device = dev.address.device;
    out_device.function = dev.address.function;
    out_device.vendor_id = dev.vendor_id;
    out_device.device_id = dev.device_id;
    out_device.class_code = dev.class_code;
    out_device.subclass = dev.subclass;
    out_device.prog_if = dev.prog_if;
    out_device.revision = dev.revision;
    out_device.header_type = dev.header_type;
    return true;
}

export fn pci_get_class_name(class_code: u8, subclass: u8, prog_if: u8) [*:0]const u8 {
    const name = getDeviceClassName(class_code, subclass, prog_if);
    return @ptrCast(name.ptr);
}

export fn pci_read_config_word(bus: u8, device: u8, function: u8, offset: u8) u16 {
    const addr = PCIAddress.init(bus, @intCast(device & 0x1F), @intCast(function & 0x07));
    return pciConfigReadWord(addr, offset);
}

export fn pci_read_config_dword(bus: u8, device: u8, function: u8, offset: u8) u32 {
    const addr = PCIAddress.init(bus, @intCast(device & 0x1F), @intCast(function & 0x07));
    return pciConfigReadDword(addr, offset);
}

export fn pci_write_config_dword(bus: u8, device: u8, function: u8, offset: u8, value: u32) void {
    const addr = PCIAddress.init(bus, @intCast(device & 0x1F), @intCast(function & 0x07));
    pciConfigWriteDword(addr, offset, value);
}
