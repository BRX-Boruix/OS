#!/bin/bash

# 快速启动测试脚本

echo "=== Boruix OS 快速启动测试 ==="
echo

echo "正在启动 x86_64 版本进行 5 秒测试..."
echo "如果看到内核输出，说明启动成功！"
echo

# 启动 x86_64 版本，5秒后自动退出
timeout 5s make ARCH=x86_64 run 2>/dev/null || echo "测试完成"

echo
echo "如果上面显示了内核启动信息，说明问题已解决！"
echo "如果仍然显示 GRUB 错误，请检查虚拟机配置。"
echo
echo "手动测试命令："
echo "  make ARCH=i386 run    # 测试32位版本"
echo "  make ARCH=x86_64 run  # 测试64位版本"
