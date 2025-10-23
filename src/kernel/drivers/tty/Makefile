# Boruix OS TTY系统 Makefile

# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -ffreestanding -fno-stack-protector -fno-stack-check -fno-pie -fno-pic -mno-red-zone -mcmodel=kernel
INCLUDES = -I../../include -I../flanterm -I../flanterm/flanterm_backends

# 源文件（完整TTY实现）
SOURCES = tty.c tty_device.c tty_session.c tty_klog.c
OBJECTS = $(SOURCES:.c=.o)

# 目标文件
TARGET = libtty.a

# 默认目标
all: $(TARGET)

# 编译静态库
$(TARGET): $(OBJECTS)
	ar rcs $@ $^

# 编译对象文件
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 清理
clean:
	rm -f $(OBJECTS) $(TARGET)

# 安装（复制到lib目录）
install: $(TARGET)
	cp $(TARGET) ../../lib/

.PHONY: all clean install
