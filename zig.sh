#!/bin/bash

# Zig 安装脚本
set -e

ZIG_URL="https://ziglang.org/builds/zig-x86_64-linux-0.16.0-dev.732+2f3234c76.tar.xz"
INSTALL_DIR="/usr/local/zig"
BIN_SYMLINK="/usr/local/bin/zig"
TEMP_DIR="/tmp/zig-install"

echo "开始安装 Zig..."

# 检查是否以 root 权限运行
if [ "$EUID" -ne 0 ]; then
    echo "请使用 sudo 运行此脚本"
    exit 1
fi

# 创建临时目录
mkdir -p "$TEMP_DIR"
cd "$TEMP_DIR"

# 下载 Zig
echo "下载 Zig..."
wget "$ZIG_URL" -O zig.tar.xz

# 解压
echo "解压文件..."
tar -xf zig.tar.xz

# 获取解压后的目录名
ZIG_DIR=$(tar -tf zig.tar.xz | head -1 | cut -f1 -d"/")

# 移除旧安装（如果存在）
echo "移除旧版本..."
rm -rf "$INSTALL_DIR"
rm -f "$BIN_SYMLINK"

# 安装到系统目录
echo "安装到 $INSTALL_DIR..."
mv "$ZIG_DIR" "$INSTALL_DIR"

# 创建符号链接
echo "创建符号链接..."
ln -sf "$INSTALL_DIR/zig" "$BIN_SYMLINK"

# 清理临时文件
echo "清理临时文件..."
rm -rf "$TEMP_DIR"

# 验证安装
echo "验证安装..."
if zig version >/dev/null 2>&1; then
    echo "✅ Zig 安装成功！"
    echo "版本信息:"
    zig version
else
    echo "❌ 安装失败"
    exit 1
fi

echo ""
echo "安装完成！现在可以在终端中使用 'zig' 命令了。"