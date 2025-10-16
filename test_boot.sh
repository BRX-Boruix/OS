#!/bin/bash

# Boruix OS 启动测试脚本

echo "=== Boruix OS 启动测试 ==="
echo

# 检查构建文件是否存在
echo "1. 检查构建文件..."
if [ -f "build/i386/boruix-i386.iso" ]; then
    echo "   ✓ i386 ISO 文件存在"
else
    echo "   ✗ i386 ISO 文件缺失"
    exit 1
fi

if [ -f "build/x86_64/boruix-x86_64.iso" ]; then
    echo "   ✓ x86_64 ISO 文件存在"
else
    echo "   ✗ x86_64 ISO 文件缺失"
    exit 1
fi

echo

# 检查内核文件
echo "2. 检查内核文件..."
file build/*/kernel.bin
echo

# 检查multiboot头
echo "3. 检查 Multiboot 头..."
echo "   i386 multiboot 头:"
hexdump -C build/i386/kernel.bin | grep "02 b0 ad 1b" | head -1
echo "   x86_64 multiboot 头:"
hexdump -C build/x86_64/kernel.bin | grep "02 b0 ad 1b" | head -1
echo

# 提供启动选项
echo "4. 启动选项:"
echo "   要测试 i386 版本，运行: make ARCH=i386 run"
echo "   要测试 x86_64 版本，运行: make ARCH=x86_64 run"
echo

echo "=== 测试完成 ==="
echo "如果上述检查都通过，系统应该能够正常启动。"
echo "如果仍然回到 GRUB，请检查虚拟机设置或尝试不同的 QEMU 参数。"
