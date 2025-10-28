// PCI驱动 - Zig构建脚本
// 适配Zig 0.16.0-dev版本
const std = @import("std");

pub fn build(b: *std.Build) void {
    // 目标平台：x86_64裸机
    const target = b.standardTargetOptions(.{});
    
    // 优化模式
    const optimize = b.standardOptimizeOption(.{});

    // 创建静态库
    const lib = b.addStaticLibrary(.{
        .name = "pci",
        .root_source_file = b.path("src/pci.zig"),
        .target = target,
        .optimize = optimize,
    });

    // 生成库文件
    b.installArtifact(lib);
}
