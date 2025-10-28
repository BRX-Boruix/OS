// ACPI最小实现 - 专注于MCFG表查找
// 用于PCI Express配置空间访问

const std = @import("std");

// ============================================================================
// ACPI结构定义
// ============================================================================

pub const ACPISDTHeader = extern struct {
    signature: [4]u8,
    length: u32,
    revision: u8,
    checksum: u8,
    oem_id: [6]u8,
    oem_table_id: [8]u8,
    oem_revision: u32,
    creator_id: u32,
    creator_revision: u32,
};

pub const RSDP = extern struct {
    signature: [8]u8,       // "RSD PTR "
    checksum: u8,
    oem_id: [6]u8,
    revision: u8,           // 0=ACPI 1.0, 2=ACPI 2.0+
    rsdt_address: u32,
    // ACPI 2.0+扩展字段
    length: u32,
    xsdt_address: u64,
    extended_checksum: u8,
    reserved: [3]u8,
};

pub const RSDT = extern struct {
    header: ACPISDTHeader,
    // entries跟在后面，数量由header.length计算
};

pub const XSDT = extern struct {
    header: ACPISDTHeader,
    // entries跟在后面，数量由header.length计算
};

pub const MCFGEntry = extern struct {
    base_addr: u64,     // ECAM基地址
    segment: u16,       // PCI段组
    start_bus: u8,      // 起始总线
    end_bus: u8,        // 结束总线
    reserved: u32,
};

pub const MCFG = extern struct {
    header: ACPISDTHeader,
    reserved: [8]u8,
    // entries跟在后面
};

// ============================================================================
// 外部函数声明
// ============================================================================

// 从Rust内存管理器获取HHDM偏移
extern fn rust_get_hhdm_offset() u64;

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
// ACPI表查找
// ============================================================================

/// 验证ACPI表校验和
fn verify_checksum(ptr: [*]const u8, length: u32) bool {
    var sum: u8 = 0;
    var i: u32 = 0;
    while (i < length) : (i += 1) {
        sum +%= ptr[i];
    }
    return sum == 0;
}

/// 比较签名
fn signature_match(sig1: []const u8, sig2: []const u8) bool {
    if (sig1.len != sig2.len) return false;
    for (sig1, 0..) |c, i| {
        if (c != sig2[i]) return false;
    }
    return true;
}

/// 在EBDA区域搜索RSDP
fn find_rsdp_in_ebda() ?*RSDP {
    // x86_64长模式：物理地址需要加上HHDM偏移（由Limine提供）
    const KERNEL_OFFSET = rust_get_hhdm_offset();
    
    // EBDA地址存储在0x40E（BDA中），需要映射到高地址
    const bda_ebda_ptr = @as(*const u16, @ptrFromInt(0x40E + KERNEL_OFFSET));
    const ebda_seg = bda_ebda_ptr.*;
    
    if (ebda_seg == 0 or ebda_seg == 0xFFFF) return null;
    
    const ebda_addr = @as(usize, ebda_seg) << 4;
    
    serial_print("[ACPI] Searching RSDP in EBDA at ");
    serial_print_hex(ebda_addr);
    serial_print("\n");
    
    // EBDA通常是1KB，搜索前1KB
    var offset: usize = 0;
    while (offset < 1024) : (offset += 16) {
        const addr = ebda_addr + offset + KERNEL_OFFSET;
        const rsdp = @as(*RSDP, @ptrFromInt(addr));
        if (signature_match(&rsdp.signature, "RSD PTR ")) {
            serial_print("[ACPI] Found RSDP at ");
            serial_print_hex(addr);
            serial_print("\n");
            return rsdp;
        }
    }
    return null;
}

/// 在BIOS区域搜索RSDP
fn find_rsdp_in_bios() ?*RSDP {
    serial_print("[ACPI] Searching RSDP in BIOS area\n");
    
    // x86_64长模式：物理地址需要加上HHDM偏移（由Limine提供）
    const KERNEL_OFFSET = rust_get_hhdm_offset();
    
    // BIOS E0000-FFFFF区域
    var addr: usize = 0xE0000;
    while (addr < 0x100000) : (addr += 16) {
        const mapped_addr = addr + KERNEL_OFFSET;
        const rsdp = @as(*RSDP, @ptrFromInt(mapped_addr));
        if (signature_match(&rsdp.signature, "RSD PTR ")) {
            serial_print("[ACPI] Found RSDP at ");
            serial_print_hex(mapped_addr);
            serial_print("\n");
            return rsdp;
        }
    }
    return null;
}

/// 查找RSDP
pub fn find_rsdp() ?*RSDP {
    serial_print("[ACPI] Starting RSDP search...\n");
    
    // 检查HHDM偏移是否已初始化
    const hhdm_offset = rust_get_hhdm_offset();
    serial_print("[ACPI] HHDM offset: ");
    serial_print_hex(hhdm_offset);
    serial_print("\n");
    
    if (hhdm_offset == 0) {
        serial_print("[ACPI] ERROR: HHDM offset is 0! Cannot access physical memory.\n");
        return null;
    }
    
    // 先在EBDA搜索
    if (find_rsdp_in_ebda()) |rsdp| {
        return rsdp;
    }
    
    // 再在BIOS区域搜索
    if (find_rsdp_in_bios()) |rsdp| {
        return rsdp;
    }
    
    serial_print("[ACPI] RSDP not found!\n");
    return null;
}

/// 在RSDT中查找表
fn find_table_in_rsdt(rsdt: *RSDT, signature: []const u8) ?*ACPISDTHeader {
    const KERNEL_OFFSET = rust_get_hhdm_offset();
    
    const entry_count = (rsdt.header.length - @sizeOf(ACPISDTHeader)) / 4;
    const entries = @as([*]u32, @ptrFromInt(@intFromPtr(rsdt) + @sizeOf(ACPISDTHeader)));
    
    serial_print("[ACPI] Searching in RSDT, entries: ");
    serial_print_hex(entry_count);
    serial_print("\n");
    
    var i: u32 = 0;
    while (i < entry_count) : (i += 1) {
        // RSDT中的地址是32位物理地址，需要映射
        const phys_addr = @as(usize, entries[i]);
        const virt_addr = phys_addr + KERNEL_OFFSET;
        const table = @as(*ACPISDTHeader, @ptrFromInt(virt_addr));
        if (signature_match(&table.signature, signature)) {
            serial_print("[ACPI] Found table: ");
            serial_print(signature);
            serial_print("\n");
            return table;
        }
    }
    return null;
}

/// 在XSDT中查找表
fn find_table_in_xsdt(xsdt: *XSDT, signature: []const u8) ?*ACPISDTHeader {
    const KERNEL_OFFSET = rust_get_hhdm_offset();
    
    const entry_count = (xsdt.header.length - @sizeOf(ACPISDTHeader)) / 8;
    const entries = @as([*]u64, @ptrFromInt(@intFromPtr(xsdt) + @sizeOf(ACPISDTHeader)));
    
    serial_print("[ACPI] Searching in XSDT, entries: ");
    serial_print_hex(entry_count);
    serial_print("\n");
    
    var i: u32 = 0;
    while (i < entry_count) : (i += 1) {
        // XSDT中的地址是64位物理地址，需要映射
        const phys_addr = entries[i];
        const virt_addr = phys_addr + KERNEL_OFFSET;
        const table = @as(*ACPISDTHeader, @ptrFromInt(virt_addr));
        if (signature_match(&table.signature, signature)) {
            serial_print("[ACPI] Found table: ");
            serial_print(signature);
            serial_print("\n");
            return table;
        }
    }
    return null;
}

/// 查找ACPI表
pub fn find_table(signature: []const u8) ?*ACPISDTHeader {
    const rsdp = find_rsdp() orelse return null;
    
    serial_print("[ACPI] RSDP revision: ");
    serial_print_hex(rsdp.revision);
    serial_print("\n");
    
    const KERNEL_OFFSET = rust_get_hhdm_offset();
    
    // 优先使用XSDT（ACPI 2.0+）
    if (rsdp.revision >= 2 and rsdp.xsdt_address != 0) {
        const xsdt_virt = rsdp.xsdt_address + KERNEL_OFFSET;
        const xsdt = @as(*XSDT, @ptrFromInt(xsdt_virt));
        serial_print("[ACPI] Using XSDT at ");
        serial_print_hex(xsdt_virt);
        serial_print("\n");
        return find_table_in_xsdt(xsdt, signature);
    }
    
    // 回退到RSDT（ACPI 1.0）
    if (rsdp.rsdt_address != 0) {
        const rsdt_virt = @as(usize, rsdp.rsdt_address) + KERNEL_OFFSET;
        const rsdt = @as(*RSDT, @ptrFromInt(rsdt_virt));
        serial_print("[ACPI] Using RSDT at ");
        serial_print_hex(rsdt_virt);
        serial_print("\n");
        return find_table_in_rsdt(rsdt, signature);
    }
    
    return null;
}

/// 查找MCFG表
pub fn find_mcfg() ?*MCFG {
    serial_print("[ACPI] Looking for MCFG table...\n");
    const header = find_table("MCFG") orelse return null;
    return @as(*MCFG, @ptrCast(header));
}

/// 获取MCFG条目数量
pub fn get_mcfg_entry_count(mcfg: *MCFG) u32 {
    const entries_size = mcfg.header.length - @sizeOf(MCFG);
    return @as(u32, @intCast(entries_size / @sizeOf(MCFGEntry)));
}

/// 获取MCFG条目
pub fn get_mcfg_entry(mcfg: *MCFG, index: u32) ?*MCFGEntry {
    const count = get_mcfg_entry_count(mcfg);
    if (index >= count) return null;
    
    const entries_ptr = @intFromPtr(mcfg) + @sizeOf(MCFG);
    const entries = @as([*]MCFGEntry, @ptrFromInt(entries_ptr));
    return &entries[index];
}

