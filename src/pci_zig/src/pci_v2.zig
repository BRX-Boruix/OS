// PCI驱动 V2 - 通用实现
// 支持：MCFG(PCI-E) + Legacy I/O双模式、动态设备列表、BAR解析

const std = @import("std");
const acpi = @import("acpi.zig");

// ============================================================================
// 常量定义
// ============================================================================

pub const PCI_COMMAND_PORT: u16 = 0xCF8;
pub const PCI_DATA_PORT: u16 = 0xCFC;

// PCI配置空间偏移
pub const PCI_CONF_VENDOR: u8 = 0x00;
pub const PCI_CONF_DEVICE: u8 = 0x02;
pub const PCI_CONF_COMMAND: u8 = 0x04;
pub const PCI_CONF_STATUS: u8 = 0x06;
pub const PCI_CONF_REVISION: u8 = 0x08;
pub const PCI_CONF_PROG_IF: u8 = 0x09;
pub const PCI_CONF_SUBCLASS: u8 = 0x0A;
pub const PCI_CONF_CLASS: u8 = 0x0B;
pub const PCI_CONF_HEADER_TYPE: u8 = 0x0E;
pub const PCI_CONF_BAR0: u8 = 0x10;
pub const PCI_CONF_BAR1: u8 = 0x14;
pub const PCI_CONF_BAR2: u8 = 0x18;
pub const PCI_CONF_BAR3: u8 = 0x1C;
pub const PCI_CONF_BAR4: u8 = 0x20;
pub const PCI_CONF_BAR5: u8 = 0x24;

// 新增：子系统和中断相关偏移
pub const PCI_CONF_SUBSYSTEM_VENDOR: u8 = 0x2C;  // Subsystem Vendor ID (16位)
pub const PCI_CONF_SUBSYSTEM_DEVICE: u8 = 0x2E;  // Subsystem Device ID (16位)
pub const PCI_CONF_INTERRUPT_LINE: u8 = 0x3C;    // Interrupt Line (8位)
pub const PCI_CONF_INTERRUPT_PIN: u8 = 0x3D;     // Interrupt Pin (8位)

pub const MAX_BUS: u16 = 256;
pub const MAX_DEVICE: u8 = 32;
pub const MAX_FUNCTION: u8 = 8;
pub const MAX_DEVICES: usize = 512;  // 增加到512个设备

// ============================================================================
// 类型定义
// ============================================================================

pub const PCIAddress = packed struct {
    function: u3,
    device: u5,
    bus: u8,
    segment: u16 = 0,  // PCI段组（MCFG需要）

    pub fn init(bus: u8, device: u5, function: u3) PCIAddress {
        return .{ .bus = bus, .device = device, .function = function, .segment = 0 };
    }
};

pub const BARType = enum(u2) {
    Memory32 = 0,
    Reserved = 1,
    Memory64 = 2,
    IO = 3,
};

pub const BAR = struct {
    bar_type: BARType,
    address: u64,
    size: u64,
    prefetchable: bool,
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
    bars: [6]?BAR,  // 最多6个BAR
    ecam_base: ?u64 = null,  // ECAM基地址（MCFG模式）
    
    // 新增：子系统和中断信息
    subsystem_vendor_id: u16 = 0,
    subsystem_device_id: u16 = 0,
    interrupt_line: u8 = 0,       // IRQ号
    interrupt_pin: u8 = 0,        // 中断脚（1=INTA, 2=INTB, 3=INTC, 4=INTD）
};

pub const DeviceClass = struct {
    code: u32,
    name: []const u8,
};

// PCI访问模式
pub const PCIMode = enum {
    Legacy,  // 传统I/O端口方式
    MCFG,    // PCI-Express ECAM方式
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

fn serial_print(s: []const u8) void {
    for (s) |c| {
        outb(COM1, c);
    }
}

fn serial_print_hex(value: u64) void {
    const hex_chars = "0123456789ABCDEF";
    var buf: [18]u8 = undefined;
    buf[0] = '0';
    buf[1] = 'x';
    var i: usize = 17;
    var v = value;
    while (i >= 2) : (i -= 1) {
        buf[i] = hex_chars[v & 0xF];
        v >>= 4;
        if (i == 2) break;
    }
    serial_print(&buf);
}

// ============================================================================
// IO端口操作（Legacy模式）
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
// PCI配置空间访问 - Legacy I/O模式
// ============================================================================

fn pci_legacy_read_dword(addr: PCIAddress, offset: u8) u32 {
    const config_addr: u32 =
        (@as(u32, 1) << 31) |
        (@as(u32, addr.bus) << 16) |
        (@as(u32, addr.device) << 11) |
        (@as(u32, addr.function) << 8) |
        (@as(u32, offset & 0xFC));
    outl(PCI_COMMAND_PORT, config_addr);
    return inl(PCI_DATA_PORT);
}

fn pci_legacy_write_dword(addr: PCIAddress, offset: u8, value: u32) void {
    const config_addr: u32 =
        (@as(u32, 1) << 31) |
        (@as(u32, addr.bus) << 16) |
        (@as(u32, addr.device) << 11) |
        (@as(u32, addr.function) << 8) |
        (@as(u32, offset & 0xFC));
    outl(PCI_COMMAND_PORT, config_addr);
    outl(PCI_DATA_PORT, value);
}

fn pci_legacy_read_word(addr: PCIAddress, offset: u8) u16 {
    const dword = pci_legacy_read_dword(addr, offset & 0xFC);
    const shift = @as(u5, @intCast((offset & 0x02) * 8));
    return @as(u16, @truncate(dword >> shift));
}

fn pci_legacy_read_byte(addr: PCIAddress, offset: u8) u8 {
    const dword = pci_legacy_read_dword(addr, offset & 0xFC);
    const shift = @as(u5, @intCast((offset & 0x03) * 8));
    return @as(u8, @truncate(dword >> shift));
}

// ============================================================================
// PCI配置空间访问 - MCFG ECAM模式
// ============================================================================

// 外部函数：获取HHDM偏移
extern fn rust_get_hhdm_offset() u64;

fn pci_ecam_address(base: u64, addr: PCIAddress, offset: u8) u64 {
    // ECAM基地址是物理地址，需要加上HHDM偏移才能访问
    const hhdm_offset = rust_get_hhdm_offset();
    const phys_addr = base +
        (@as(u64, addr.bus) << 20) |
        (@as(u64, addr.device) << 15) |
        (@as(u64, addr.function) << 12) |
        @as(u64, offset);
    return phys_addr + hhdm_offset;
}

fn pci_ecam_read_dword(base: u64, addr: PCIAddress, offset: u8) u32 {
    const ecam_addr = pci_ecam_address(base, addr, offset & 0xFC);
    const ptr = @as(*volatile u32, @ptrFromInt(ecam_addr));
    return ptr.*;
}

fn pci_ecam_write_dword(base: u64, addr: PCIAddress, offset: u8, value: u32) void {
    const ecam_addr = pci_ecam_address(base, addr, offset & 0xFC);
    const ptr = @as(*volatile u32, @ptrFromInt(ecam_addr));
    ptr.* = value;
}

fn pci_ecam_read_word(base: u64, addr: PCIAddress, offset: u8) u16 {
    const dword = pci_ecam_read_dword(base, addr, offset & 0xFC);
    const shift = @as(u5, @intCast((offset & 0x02) * 8));
    return @as(u16, @truncate(dword >> shift));
}

fn pci_ecam_read_byte(base: u64, addr: PCIAddress, offset: u8) u8 {
    const dword = pci_ecam_read_dword(base, addr, offset & 0xFC);
    const shift = @as(u5, @intCast((offset & 0x03) * 8));
    return @as(u8, @truncate(dword >> shift));
}

// ============================================================================
// PCI统一访问接口（自动选择模式）
// ============================================================================

var pci_mode: PCIMode = PCIMode.Legacy;
var mcfg_table: ?*acpi.MCFG = null;
var mcfg_entry_count: u32 = 0;

// Segment支持：为每个Segment存储ECAM基地址
var mcfg_entries: [8]*acpi.MCFGEntry = undefined;  // 最多8个Segment
var segment_count: u32 = 0;

fn find_mcfg_entry_for_bus_segment(bus: u8, segment: u16) ?*acpi.MCFGEntry {
    const mcfg = mcfg_table orelse return null;
    
    var i: u32 = 0;
    while (i < mcfg_entry_count) : (i += 1) {
        const entry = acpi.get_mcfg_entry(mcfg, i) orelse continue;
        // 匹配segment和bus范围
        if (entry.segment == segment and bus >= entry.start_bus and bus <= entry.end_bus) {
            return entry;
        }
    }
    return null;
}

// 向后兼容：默认使用segment 0
fn find_mcfg_entry_for_bus(bus: u8) ?*acpi.MCFGEntry {
    return find_mcfg_entry_for_bus_segment(bus, 0);
}

pub fn pci_config_read_dword(addr: PCIAddress, offset: u8) u32 {
    if (pci_mode == PCIMode.MCFG) {
        if (find_mcfg_entry_for_bus(addr.bus)) |entry| {
            return pci_ecam_read_dword(entry.base_addr, addr, offset);
        }
    }
    return pci_legacy_read_dword(addr, offset);
}

pub fn pci_config_write_dword(addr: PCIAddress, offset: u8, value: u32) void {
    if (pci_mode == PCIMode.MCFG) {
        if (find_mcfg_entry_for_bus(addr.bus)) |entry| {
            pci_ecam_write_dword(entry.base_addr, addr, offset, value);
            return;
        }
    }
    pci_legacy_write_dword(addr, offset, value);
}

pub fn pci_config_write_word(addr: PCIAddress, offset: u8, value: u16) void {
    const dword_offset = offset & 0xFC;
    const byte_offset = offset & 0x03;
    const shift = @as(u5, @intCast(byte_offset * 8));
    
    var dword = pci_config_read_dword(addr, dword_offset);
    const mask: u32 = 0xFFFF << shift;
    dword = (dword & ~mask) | (@as(u32, value) << shift);
    pci_config_write_dword(addr, dword_offset, dword);
}

pub fn pci_config_read_word(addr: PCIAddress, offset: u8) u16 {
    if (pci_mode == PCIMode.MCFG) {
        if (find_mcfg_entry_for_bus(addr.bus)) |entry| {
            return pci_ecam_read_word(entry.base_addr, addr, offset);
        }
    }
    return pci_legacy_read_word(addr, offset);
}

pub fn pci_config_read_byte(addr: PCIAddress, offset: u8) u8 {
    if (pci_mode == PCIMode.MCFG) {
        if (find_mcfg_entry_for_bus(addr.bus)) |entry| {
            return pci_ecam_read_byte(entry.base_addr, addr, offset);
        }
    }
    return pci_legacy_read_byte(addr, offset);
}

// ============================================================================
// PCI Capability支持
// ============================================================================

pub const PCI_CAP_POINTER: u8 = 0x34;  // Capability指针（8位）

pub const PCI_STATUS_CAP_LIST: u16 = 0x0010;  // 支持Capability列表

// Capability ID定义
pub const CAP_ID_MSI: u8 = 0x05;           // Message Signaled Interrupts
pub const CAP_ID_MSIX: u8 = 0x11;          // MSI-X
pub const CAP_ID_PM: u8 = 0x01;            // Power Management
pub const CAP_ID_VPD: u8 = 0x03;           // Vital Product Data
pub const CAP_ID_SLOTID: u8 = 0x04;        // Slot Identification
pub const CAP_ID_PCIE: u8 = 0x10;          // PCI Express
pub const CAP_ID_PCIX: u8 = 0x07;          // PCI-X
pub const CAP_ID_HT: u8 = 0x08;            // HyperTransport
pub const CAP_ID_VENDOR: u8 = 0x09;        // Vendor Specific
pub const CAP_ID_DEBUG: u8 = 0x0A;         // Debug Port
pub const CAP_ID_CPCI_HOTSWAP: u8 = 0x0C; // CompactPCI Hot Swap
pub const CAP_ID_SHPC: u8 = 0x0C;          // Standard Hot Plug Controller
pub const CAP_ID_SSVID: u8 = 0x0D;         // Subsystem Vendor ID

pub const Capability = struct {
    id: u8,                    // Capability ID
    offset: u8,                // 配置空间偏移
    next_offset: u8,           // 下一个Capability的偏移
};

// 最多支持16个Capability
pub const MAX_CAPABILITIES: usize = 16;

pub const CapabilityList = struct {
    capabilities: [MAX_CAPABILITIES]?Capability = [_]?Capability{null} ** MAX_CAPABILITIES,
    count: usize = 0,
    
    pub fn has_msi(self: *const CapabilityList) bool {
        return self.find(CAP_ID_MSI) != null;
    }
    
    pub fn has_msix(self: *const CapabilityList) bool {
        return self.find(CAP_ID_MSIX) != null;
    }
    
    pub fn has_pcie(self: *const CapabilityList) bool {
        return self.find(CAP_ID_PCIE) != null;
    }
    
    pub fn find(self: *const CapabilityList, cap_id: u8) ?*const Capability {
        for (self.capabilities) |cap_opt| {
            if (cap_opt) |cap| {
                if (cap.id == cap_id) {
                    return &cap;
                }
            }
        }
        return null;
    }
};

// ============================================================================
// MSI支持
// ============================================================================

pub const MSICapability = struct {
    offset: u8,                 // MSI Capability在配置空间的偏移
    control: u16,              // Message Control寄存器
    is_64bit: bool,            // 是否支持64位地址
    multiple_messages: u8,      // 支持的消息数（2^N）
    enabled: bool,              // 是否启用
};

// MSI Control寄存器位定义
pub const MSI_CONTROL_ENABLE: u16 = 0x0001;           // MSI启用
pub const MSI_CONTROL_MULTI_MSG_CAPABLE: u16 = 0x000E; // 支持的消息数(bits 3-1)
pub const MSI_CONTROL_MULTI_MSG_ENABLED: u16 = 0x0070; // 启用的消息数(bits 6-4)
pub const MSI_CONTROL_64BIT: u16 = 0x0080;            // 64位地址支持

/// 读取MSI Capability信息
pub fn read_msi_capability(addr: PCIAddress, cap_offset: u8) ?MSICapability {
    // 读取MSI Control寄存器
    const control = pci_config_read_word(addr, cap_offset + 2);
    
    // 提取信息
    const is_64bit = (control & MSI_CONTROL_64BIT) != 0;
    const multiple_msg_capable = @as(u8, @truncate((control & MSI_CONTROL_MULTI_MSG_CAPABLE) >> 1));
    const enabled = (control & MSI_CONTROL_ENABLE) != 0;
    
    return MSICapability{
        .offset = cap_offset,
        .control = control,
        .is_64bit = is_64bit,
        .multiple_messages = multiple_msg_capable,
        .enabled = enabled,
    };
}

/// 启用MSI
pub fn enable_msi(addr: PCIAddress, msi: *MSICapability, vector: u8) void {
    // 设置Message Address寄存器
    const msg_addr_offset = msi.offset + 4;
    if (msi.is_64bit) {
        // 64位地址: 0xFEE00000 + (APIC ID << 12)
        const addr_low: u32 = 0xFEE00000;
        const addr_high: u32 = 0;
        pci_config_write_dword(addr, msg_addr_offset, addr_low);
        pci_config_write_dword(addr, msg_addr_offset + 4, addr_high);
        
        // Message Data寄存器在64位BAR后面 (offset + 12)
        const msg_data_offset = msg_addr_offset + 8;
        pci_config_write_word(addr, msg_data_offset, @as(u16, vector));
    } else {
        // 32位地址
        pci_config_write_dword(addr, msg_addr_offset, 0xFEE00000);
        
        // Message Data寄存器在32位BAR后面 (offset + 8)
        const msg_data_offset = msg_addr_offset + 4;
        pci_config_write_word(addr, msg_data_offset, @as(u16, vector));
    }
    
    // 启用MSI
    var control = msi.control;
    control |= MSI_CONTROL_ENABLE;
    pci_config_write_word(addr, msi.offset + 2, control);
    msi.enabled = true;
}

// ============================================================================
// 扫描设备的Capability列表
// ============================================================================

/// 扫描设备的Capability列表
pub fn scan_capabilities(addr: PCIAddress) CapabilityList {
    var cap_list = CapabilityList{};
    
    // 检查设备是否支持Capability列表
    const status = pci_config_read_word(addr, PCI_CONF_STATUS);
    if ((status & PCI_STATUS_CAP_LIST) == 0) {
        return cap_list;  // 不支持Capability列表
    }
    
    // 获取第一个Capability指针
    var cap_offset = pci_config_read_byte(addr, PCI_CAP_POINTER) & 0xFC;
    
    // 遍历Capability链表
    var visited_count: u32 = 0;
    while (cap_offset != 0 and visited_count < 32) : (visited_count += 1) {
        // 防止无限循环
        if (cap_offset < 0x40) break;  // Capability必须在0x40之后
        
        // 读取Capability ID和下一个指针
        const cap_header = pci_config_read_byte(addr, cap_offset);
        const cap_id = cap_header;
        const next_offset = pci_config_read_byte(addr, cap_offset + 1) & 0xFC;
        
        if (cap_list.count < MAX_CAPABILITIES) {
            cap_list.capabilities[cap_list.count] = Capability{
                .id = cap_id,
                .offset = cap_offset,
                .next_offset = next_offset,
            };
            cap_list.count += 1;
        }
        
        cap_offset = next_offset;
        if (cap_offset == 0) break;  // 链表结束
    }
    
    return cap_list;
}

// ============================================================================
// PCI设备检测和扫描
// ============================================================================

pub fn pci_device_exists(addr: PCIAddress) bool {
    const vendor_id = pci_config_read_word(addr, PCI_CONF_VENDOR);
    return vendor_id != 0xFFFF and vendor_id != 0x0000;
}

pub fn pci_is_multifunction(addr: PCIAddress) bool {
    const header_type = pci_config_read_byte(addr, PCI_CONF_HEADER_TYPE);
    return (header_type & 0x80) != 0;
}

// ============================================================================
// BAR解析和大小计算
// ============================================================================

/// 计算BAR大小（通过-1探测）
fn calculate_bar_size(addr: PCIAddress, bar_offset: u8, bar_type: BARType) u64 {
    // 保存原始值
    const original_value = pci_config_read_dword(addr, bar_offset);
    
    // 写入-1（全1）到BAR
    pci_config_write_dword(addr, bar_offset, 0xFFFFFFFF);
    
    // 读取探测结果
    const probe_value = pci_config_read_dword(addr, bar_offset);
    
    // 恢复原始值
    pci_config_write_dword(addr, bar_offset, original_value);
    
    // 计算大小
    // 对于IO BAR: mask = 0xFFFFFFFC（最低2位是标志）
    // 对于Memory BAR: mask = 0xFFFFFFF0（最低4位是标志）
    
    if (bar_type == BARType.IO) {
        const mask = probe_value & 0xFFFFFFFC;
        if (mask == 0) return 0;
        return (~mask +% 1) & 0xFFFFFFFF;
    } else {
        const mask = probe_value & 0xFFFFFFF0;
        if (mask == 0) return 0;
        return (~mask +% 1) & 0xFFFFFFFF;
    }
}

/// 计算64位BAR大小（两个32位寄存器）
fn calculate_bar_size_64(addr: PCIAddress, bar_offset: u8) u64 {
    // 保存原始值
    const original_low = pci_config_read_dword(addr, bar_offset);
    const original_high = pci_config_read_dword(addr, bar_offset + 4);
    
    // 写入-1到两个32位寄存器
    pci_config_write_dword(addr, bar_offset, 0xFFFFFFFF);
    pci_config_write_dword(addr, bar_offset + 4, 0xFFFFFFFF);
    
    // 读取探测结果
    const probe_low = pci_config_read_dword(addr, bar_offset);
    const probe_high = pci_config_read_dword(addr, bar_offset + 4);
    
    // 恢复原始值
    pci_config_write_dword(addr, bar_offset, original_low);
    pci_config_write_dword(addr, bar_offset + 4, original_high);
    
    // 计算大小：mask = probe值 & 0xFFFFFFF0（低位是标志）
    const mask_low = probe_low & 0xFFFFFFF0;
    const mask_high = probe_high;
    
    if (mask_low == 0 and mask_high == 0) return 0;
    
    const mask: u64 = (@as(u64, mask_high) << 32) | mask_low;
    return (~mask +% 1);
}

fn parse_bar(addr: PCIAddress, bar_index: u8) ?BAR {
    if (bar_index >= 6) return null;
    
    const bar_offset = PCI_CONF_BAR0 + (bar_index * 4);
    const bar_value = pci_config_read_dword(addr, bar_offset);
    
    if (bar_value == 0) return null;
    
    // 判断IO还是内存映射
    const is_io = (bar_value & 0x01) != 0;
    
    if (is_io) {
        // IO空间BAR
        const size = calculate_bar_size(addr, bar_offset, BARType.IO);
        return BAR{
            .bar_type = BARType.IO,
            .address = bar_value & 0xFFFFFFFC,
            .size = size,
            .prefetchable = false,
        };
    } else {
        // 内存空间BAR
        const bar_type_bits = (bar_value >> 1) & 0x03;
        const prefetchable = (bar_value & 0x08) != 0;
        
        if (bar_type_bits == 0) {
            // 32位地址
            const size = calculate_bar_size(addr, bar_offset, BARType.Memory32);
            return BAR{
                .bar_type = BARType.Memory32,
                .address = bar_value & 0xFFFFFFF0,
                .size = size,
                .prefetchable = prefetchable,
            };
        } else if (bar_type_bits == 2) {
            // 64位地址
            if (bar_index >= 5) return null;  // 64位BAR需要两个寄存器
            
            const bar_high = pci_config_read_dword(addr, bar_offset + 4);
            const address = (@as(u64, bar_high) << 32) | (bar_value & 0xFFFFFFF0);
            const size = calculate_bar_size_64(addr, bar_offset);
            
            return BAR{
                .bar_type = BARType.Memory64,
                .address = address,
                .size = size,
                .prefetchable = prefetchable,
            };
        }
    }
    
    return null;
}

// ============================================================================
// 设备类别名称映射（扩展版）
// ============================================================================

const device_classes = [_]DeviceClass{
    // 00: 未分类设备
    .{ .code = 0x000000, .name = "Non-VGA Unclassified Device" },
    .{ .code = 0x000100, .name = "VGA-Compatible Unclassified Device" },
    
    // 01: 存储控制器
    .{ .code = 0x010000, .name = "SCSI Bus Controller" },
    .{ .code = 0x010100, .name = "IDE Controller" },
    .{ .code = 0x010200, .name = "Floppy Disk Controller" },
    .{ .code = 0x010300, .name = "IPI Bus Controller" },
    .{ .code = 0x010400, .name = "RAID Controller" },
    .{ .code = 0x010500, .name = "ATA Controller" },
    .{ .code = 0x010600, .name = "SATA Controller" },
    .{ .code = 0x010700, .name = "Serial Attached SCSI Controller" },
    .{ .code = 0x010800, .name = "NVMe Controller" },
    
    // 02: 网络控制器
    .{ .code = 0x020000, .name = "Ethernet Controller" },
    .{ .code = 0x020100, .name = "Token Ring Controller" },
    .{ .code = 0x020200, .name = "FDDI Controller" },
    .{ .code = 0x020300, .name = "ATM Controller" },
    .{ .code = 0x020400, .name = "ISDN Controller" },
    .{ .code = 0x020600, .name = "PICMG 2.14 Multi Computing" },
    .{ .code = 0x020700, .name = "InfiniBand Controller" },
    .{ .code = 0x020800, .name = "Fabric Controller" },
    
    // 03: 显示控制器
    .{ .code = 0x030000, .name = "VGA Controller" },
    .{ .code = 0x030001, .name = "8514-Compatible Controller" },
    .{ .code = 0x030100, .name = "XGA Controller" },
    .{ .code = 0x030200, .name = "3D Controller" },
    
    // 04: 多媒体设备
    .{ .code = 0x040000, .name = "Multimedia Video Controller" },
    .{ .code = 0x040100, .name = "Multimedia Audio Controller" },
    .{ .code = 0x040200, .name = "Computer Telephony Device" },
    .{ .code = 0x040300, .name = "Audio Device" },
    
    // 05: 内存控制器
    .{ .code = 0x050000, .name = "RAM Controller" },
    .{ .code = 0x050100, .name = "Flash Controller" },
    
    // 06: 桥设备
    .{ .code = 0x060000, .name = "Host Bridge" },
    .{ .code = 0x060100, .name = "ISA Bridge" },
    .{ .code = 0x060200, .name = "EISA Bridge" },
    .{ .code = 0x060300, .name = "MCA Bridge" },
    .{ .code = 0x060400, .name = "PCI-to-PCI Bridge" },
    .{ .code = 0x060401, .name = "PCI-to-PCI Bridge (Subtractive Decode)" },
    .{ .code = 0x060500, .name = "PCMCIA Bridge" },
    .{ .code = 0x060600, .name = "NuBus Bridge" },
    .{ .code = 0x060700, .name = "CardBus Bridge" },
    .{ .code = 0x060800, .name = "RACEway Bridge" },
    .{ .code = 0x060900, .name = "PCI-to-PCI Bridge (Semi-Transparent)" },
    .{ .code = 0x060A00, .name = "InfiniBand-to-PCI Bridge" },
    
    // 07: 通信控制器
    .{ .code = 0x070000, .name = "Serial Controller" },
    .{ .code = 0x070001, .name = "16450-Compatible Serial Controller" },
    .{ .code = 0x070002, .name = "16550-Compatible Serial Controller" },
    .{ .code = 0x070100, .name = "Parallel Controller" },
    .{ .code = 0x070200, .name = "Multiport Serial Controller" },
    .{ .code = 0x070300, .name = "Modem" },
    .{ .code = 0x070400, .name = "GPIB Controller" },
    .{ .code = 0x070500, .name = "Smart Card Controller" },
    
    // 08: 系统外设
    .{ .code = 0x080000, .name = "PIC" },
    .{ .code = 0x080100, .name = "DMA Controller" },
    .{ .code = 0x080200, .name = "Timer" },
    .{ .code = 0x080300, .name = "RTC Controller" },
    .{ .code = 0x080400, .name = "PCI Hot-Plug Controller" },
    .{ .code = 0x080500, .name = "SD Host Controller" },
    .{ .code = 0x080600, .name = "IOMMU" },
    
    // 09: 输入设备
    .{ .code = 0x090000, .name = "Keyboard Controller" },
    .{ .code = 0x090100, .name = "Digitizer Pen" },
    .{ .code = 0x090200, .name = "Mouse Controller" },
    .{ .code = 0x090300, .name = "Scanner Controller" },
    .{ .code = 0x090400, .name = "Gameport Controller" },
    
    // 0C: 串行总线控制器
    .{ .code = 0x0c0000, .name = "FireWire (IEEE 1394) Controller" },
    .{ .code = 0x0c0100, .name = "ACCESS Bus Controller" },
    .{ .code = 0x0c0200, .name = "SSA Controller" },
    .{ .code = 0x0c0300, .name = "USB Controller (UHCI)" },
    .{ .code = 0x0c0310, .name = "USB Controller (OHCI)" },
    .{ .code = 0x0c0320, .name = "USB Controller (EHCI)" },
    .{ .code = 0x0c0330, .name = "USB Controller (XHCI)" },
    .{ .code = 0x0c0400, .name = "Fibre Channel Controller" },
    .{ .code = 0x0c0500, .name = "SMBus Controller" },
    .{ .code = 0x0c0600, .name = "InfiniBand Controller" },
    .{ .code = 0x0c0700, .name = "IPMI Interface" },
    .{ .code = 0x0c0800, .name = "SERCOS Interface" },
    .{ .code = 0x0c0900, .name = "CANbus Controller" },
};

pub fn get_device_class_name(class_code: u8, subclass: u8, prog_if: u8) []const u8 {
    const full_code = (@as(u32, class_code) << 16) |
        (@as(u32, subclass) << 8) |
        @as(u32, prog_if);

    // 先精确匹配（包含prog_if）
    for (device_classes) |dc| {
        if (dc.code == full_code) {
            return dc.name;
        }
    }

    // 再匹配class+subclass
    const partial_code = full_code & 0xFFFF00;
    for (device_classes) |dc| {
        if (dc.code == partial_code) {
            return dc.name;
        }
    }

    // 最后只匹配class
    const class_only = full_code & 0xFF0000;
    for (device_classes) |dc| {
        if (dc.code == class_only) {
            return dc.name;
        }
    }

    return "Unknown Device";
}

// ============================================================================
// 设备读取
// ============================================================================

pub fn pci_read_device(addr: PCIAddress) PCIDevice {
    const vendor_device = pci_config_read_dword(addr, PCI_CONF_VENDOR);
    const vendor_id = @as(u16, @truncate(vendor_device));
    const device_id = @as(u16, @truncate(vendor_device >> 16));

    const class_info = pci_config_read_dword(addr, PCI_CONF_REVISION);
    const revision = @as(u8, @truncate(class_info));
    const prog_if = @as(u8, @truncate(class_info >> 8));
    const subclass = @as(u8, @truncate(class_info >> 16));
    const class_code = @as(u8, @truncate(class_info >> 24));

    const header_type = pci_config_read_byte(addr, PCI_CONF_HEADER_TYPE);

    // 解析BAR
    var bars: [6]?BAR = [_]?BAR{null} ** 6;
    var i: u8 = 0;
    while (i < 6) : (i += 1) {
        bars[i] = parse_bar(addr, i);
        // 如果是64位BAR，跳过下一个
        if (bars[i]) |bar| {
            if (bar.bar_type == BARType.Memory64) {
                i += 1;
            }
        }
    }

    // 获取ECAM基地址
    var ecam_base: ?u64 = null;
    if (find_mcfg_entry_for_bus(addr.bus)) |entry| {
        ecam_base = entry.base_addr;
    }

    // 读取子系统ID
    const subsystem_info = pci_config_read_dword(addr, PCI_CONF_SUBSYSTEM_VENDOR);
    const subsystem_vendor_id = @as(u16, @truncate(subsystem_info));
    const subsystem_device_id = @as(u16, @truncate(subsystem_info >> 16));

    // 读取中断信息
    const interrupt_line = pci_config_read_byte(addr, PCI_CONF_INTERRUPT_LINE);
    const interrupt_pin = pci_config_read_byte(addr, PCI_CONF_INTERRUPT_PIN);

    return PCIDevice{
        .address = addr,
        .vendor_id = vendor_id,
        .device_id = device_id,
        .class_code = class_code,
        .subclass = subclass,
        .prog_if = prog_if,
        .revision = revision,
        .header_type = header_type & 0x7F,
        .bars = bars,
        .ecam_base = ecam_base,
        .subsystem_vendor_id = subsystem_vendor_id,
        .subsystem_device_id = subsystem_device_id,
        .interrupt_line = interrupt_line,
        .interrupt_pin = interrupt_pin,
    };
}

// ============================================================================
// 动态设备列表
// ============================================================================

var device_list: [MAX_DEVICES]PCIDevice = undefined;
var device_count: usize = 0;

// ============================================================================
// PCI初始化和扫描
// ============================================================================

export fn pci_init() void {
    serial_print("\n[PCI] ========== PCI Driver V2 ==========\n");
    device_count = 0;
    
    // 尝试启用MCFG（如果HHDM不可用会自动回退到Legacy）
    serial_print("[PCI] Checking for MCFG support...\n");
    if (acpi.find_mcfg()) |mcfg| {
        mcfg_table = mcfg;
        mcfg_entry_count = acpi.get_mcfg_entry_count(mcfg);
        pci_mode = PCIMode.MCFG;
        
        serial_print("[PCI] MCFG enabled with ");
        serial_print_hex(mcfg_entry_count);
        serial_print(" entries\n");
        
        // 收集所有Segment信息
        segment_count = 0;
        var i: u32 = 0;
        while (i < mcfg_entry_count) : (i += 1) {
            if (acpi.get_mcfg_entry(mcfg, i)) |entry| {
                serial_print("[PCI] MCFG[");
                serial_print_hex(i);
                serial_print("] base=");
                serial_print_hex(entry.base_addr);
                serial_print(" segment=");
                serial_print_hex(entry.segment);
                serial_print(" bus=");
                serial_print_hex(entry.start_bus);
                serial_print("-");
                serial_print_hex(entry.end_bus);
                serial_print("\n");
                
                if (segment_count < 8) {
                    mcfg_entries[segment_count] = entry;
                    segment_count += 1;
                }
            }
        }
    } else {
        serial_print("[PCI] MCFG not found, using Legacy I/O mode\n");
        pci_mode = PCIMode.Legacy;
    }
    
    // 扫描设备
    serial_print("[PCI] Starting device scan...\n");
    var bus: u16 = 0;
    const max_bus: u16 = if (pci_mode == PCIMode.MCFG) 256 else 16;
    
    while (bus < max_bus) : (bus += 1) {
        var dev: u8 = 0;
        while (dev < MAX_DEVICE) : (dev += 1) {
            const addr = PCIAddress.init(@intCast(bus), @intCast(dev), 0);
            
            if (!pci_device_exists(addr)) continue;

            const device = pci_read_device(addr);
            if (device_count < device_list.len) {
                device_list[device_count] = device;
                device_count += 1;
            }

            if (pci_is_multifunction(addr)) {
                var func: u8 = 1;
                while (func < MAX_FUNCTION) : (func += 1) {
                    const func_addr = PCIAddress.init(@intCast(bus), @intCast(dev), @intCast(func & 0x07));
                    
                    const vendor = pci_config_read_word(func_addr, PCI_CONF_VENDOR);
                    if (vendor == 0xFFFF or vendor == 0x0000) continue;
                    
                    const func_device = pci_read_device(func_addr);
                    if (device_count < device_list.len) {
                        device_list[device_count] = func_device;
                        device_count += 1;
                    }
                }
            }
        }
    }
    
    serial_print("[PCI] Scan complete, found ");
    serial_print_hex(device_count);
    serial_print(" devices\n");
    serial_print("[PCI] ====================================\n\n");
}

// ============================================================================
// C FFI导出接口（兼容现有代码）
// ============================================================================

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
    subsystem_vendor_id: u16,
    subsystem_device_id: u16,
    interrupt_line: u8,
    interrupt_pin: u8,
};

export fn pci_get_device_count() usize {
    return device_count;
}

/// 获取PCI Segment数量（多Segment支持）
export fn pci_get_segment_count() u32 {
    return segment_count;
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
    out_device.subsystem_vendor_id = dev.subsystem_vendor_id;
    out_device.subsystem_device_id = dev.subsystem_device_id;
    out_device.interrupt_line = dev.interrupt_line;
    out_device.interrupt_pin = dev.interrupt_pin;
    return true;
}

export fn pci_get_class_name(class_code: u8, subclass: u8, prog_if: u8) [*:0]const u8 {
    const name = get_device_class_name(class_code, subclass, prog_if);
    return @ptrCast(name.ptr);
}

export fn pci_read_config_word(bus: u8, device: u8, function: u8, offset: u8) u16 {
    const addr = PCIAddress.init(bus, @intCast(device & 0x1F), @intCast(function & 0x07));
    return pci_config_read_word(addr, offset);
}

export fn pci_read_config_dword(bus: u8, device: u8, function: u8, offset: u8) u32 {
    const addr = PCIAddress.init(bus, @intCast(device & 0x1F), @intCast(function & 0x07));
    return pci_config_read_dword(addr, offset);
}

export fn pci_write_config_dword(bus: u8, device: u8, function: u8, offset: u8, value: u32) void {
    const addr = PCIAddress.init(bus, @intCast(device & 0x1F), @intCast(function & 0x07));
    pci_config_write_dword(addr, offset, value);
}

// 新增：获取PCI模式
export fn pci_get_mode() u8 {
    return switch (pci_mode) {
        PCIMode.Legacy => 0,
        PCIMode.MCFG => 1,
    };
}

// 新增：获取设备BAR信息
export fn pci_get_bar(index: usize, bar_idx: u8, out_addr: *u64, out_size: *u64) bool {
    if (index >= device_count or bar_idx >= 6) return false;
    
    const dev = &device_list[index];
    if (dev.bars[bar_idx]) |bar| {
        out_addr.* = bar.address;
        out_size.* = bar.size;
        return true;
    }
    return false;
}

// C结构体定义：BAR信息（与C头文件对应）
pub const PCI_BAR_C = extern struct {
    address: u64,
    size: u64,
    bar_type: u8,      // 0=Mem32, 2=Mem64, 3=IO
    prefetchable: u8,
};

/// 获取完整BAR信息（含类型和标志）
export fn pci_get_bar_info(device_index: usize, bar_index: u8, out_bar: *PCI_BAR_C) bool {
    if (device_index >= device_count or bar_index >= 6) return false;
    
    const dev = &device_list[device_index];
    if (dev.bars[bar_index]) |bar| {
        out_bar.address = bar.address;
        out_bar.size = bar.size;
        
        // 转换BAR类型为C值
        out_bar.bar_type = switch (bar.bar_type) {
            BARType.IO => 3,
            BARType.Memory32 => 0,
            BARType.Memory64 => 2,
            else => 0xFF,
        };
        
        out_bar.prefetchable = if (bar.prefetchable) 1 else 0;
        return true;
    }
    return false;
}

/// 获取设备的子系统信息
export fn pci_get_subsystem_info(device_index: usize, out_vendor: *u16, out_device: *u16) bool {
    if (device_index >= device_count) return false;
    
    const dev = &device_list[device_index];
    out_vendor.* = dev.subsystem_vendor_id;
    out_device.* = dev.subsystem_device_id;
    return true;
}

/// 获取设备的中断信息
export fn pci_get_interrupt_info(device_index: usize, out_line: *u8, out_pin: *u8) bool {
    if (device_index >= device_count) return false;
    
    const dev = &device_list[device_index];
    out_line.* = dev.interrupt_line;
    out_pin.* = dev.interrupt_pin;
    return true;
}

// ============================================================================
// 设备过滤和查询API
// ============================================================================

/// 按Vendor ID查找设备
export fn pci_find_by_vendor(vendor_id: u16, out_indices: [*]usize, max_count: usize) usize {
    var found: usize = 0;
    for (0..device_count) |i| {
        if (found >= max_count) break;
        if (device_list[i].vendor_id == vendor_id) {
            out_indices[found] = i;
            found += 1;
        }
    }
    return found;
}

/// 按Device ID查找设备
export fn pci_find_by_device(device_id: u16, out_indices: [*]usize, max_count: usize) usize {
    var found: usize = 0;
    for (0..device_count) |i| {
        if (found >= max_count) break;
        if (device_list[i].device_id == device_id) {
            out_indices[found] = i;
            found += 1;
        }
    }
    return found;
}

/// 按Vendor和Device ID同时查找（精确匹配）
export fn pci_find_by_vendor_and_device(vendor_id: u16, device_id: u16, out_indices: [*]usize, max_count: usize) usize {
    var found: usize = 0;
    for (0..device_count) |i| {
        if (found >= max_count) break;
        if (device_list[i].vendor_id == vendor_id and device_list[i].device_id == device_id) {
            out_indices[found] = i;
            found += 1;
        }
    }
    return found;
}

/// 按Class Code查找设备
export fn pci_find_by_class(class_code: u8, out_indices: [*]usize, max_count: usize) usize {
    var found: usize = 0;
    for (0..device_count) |i| {
        if (found >= max_count) break;
        if (device_list[i].class_code == class_code) {
            out_indices[found] = i;
            found += 1;
        }
    }
    return found;
}

/// 按Class和SubClass查找设备
export fn pci_find_by_class_and_subclass(class_code: u8, subclass: u8, out_indices: [*]usize, max_count: usize) usize {
    var found: usize = 0;
    for (0..device_count) |i| {
        if (found >= max_count) break;
        if (device_list[i].class_code == class_code and device_list[i].subclass == subclass) {
            out_indices[found] = i;
            found += 1;
        }
    }
    return found;
}

/// 按Bus号查找设备
export fn pci_find_by_bus(bus: u8, out_indices: [*]usize, max_count: usize) usize {
    var found: usize = 0;
    for (0..device_count) |i| {
        if (found >= max_count) break;
        if (device_list[i].address.bus == bus) {
            out_indices[found] = i;
            found += 1;
        }
    }
    return found;
}

