# Computer:/ 文件系统 Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O0
INCLUDES = -I. -Ivfs_core -Idevice_layer -Istate_files

# 源文件
VFS_SOURCES = vfs_core/computer_vfs.c
DEVICE_SOURCES = device_layer/device_interface.c
STATUS_SOURCES = state_files/status_reader.c
TOOL_SOURCES = tools/fs_debug.c

# 目标文件
VFS_OBJECTS = $(VFS_SOURCES:.c=.o)
DEVICE_OBJECTS = $(DEVICE_SOURCES:.c=.o)
STATUS_OBJECTS = $(STATUS_SOURCES:.c=.o)
TOOL_OBJECTS = $(TOOL_SOURCES:.c=.o)

# 可执行文件
TARGETS = fs_debug

.PHONY: all clean test install

all: $(TARGETS)

# 编译调试工具
fs_debug: $(VFS_OBJECTS) $(TOOL_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# 编译规则
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 清理
clean:
	rm -f $(VFS_OBJECTS) $(DEVICE_OBJECTS) $(STATUS_OBJECTS) $(TOOL_OBJECTS)
	rm -f $(TARGETS)

# 测试
test: fs_debug
	@echo "运行基础测试..."
	./fs_debug --init
	./fs_debug --tree
	./fs_debug --status

# 安装 (暂未实现)
install:
	@echo "安装功能暂未实现"

# 帮助
help:
	@echo "可用目标:"
	@echo "  all     - 编译所有目标"
	@echo "  clean   - 清理编译文件"
	@echo "  test    - 运行测试"
	@echo "  install - 安装 (暂未实现)"
	@echo "  help    - 显示此帮助"
