# Makefile for Boruix OS
# 支持i386架构，使用NASM和GCC
# 重新组织的项目结构

# 编译器设置
AS = nasm
CC = gcc
LD = ld

# 编译标志
ASFLAGS = -f elf32
CFLAGS = -m32 -std=c99 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -Wextra
LDFLAGS = -m elf_i386 -T $(BUILD_DIR)/linker.ld

# 目录设置
SRC_DIR = src
BUILD_DIR = build
CONFIG_DIR = config
DOCS_DIR = docs
SCRIPTS_DIR = scripts

# 目标文件
BOOTLOADER = $(BUILD_DIR)/boot.bin
KERNEL = $(BUILD_DIR)/kernel.bin
ISO = $(BUILD_DIR)/boruix.iso

# 源文件
BOOTLOADER_SRC = $(SRC_DIR)/boot/boot.asm
KERNEL_SRCS = $(SRC_DIR)/kernel/kernel/core/main.c \
              $(SRC_DIR)/kernel/kernel/core/multiboot.c \
              $(SRC_DIR)/kernel/drivers/display/display.c \
              $(SRC_DIR)/kernel/drivers/cmos/cmos.c \
              $(SRC_DIR)/kernel/drivers/keyboard/keyboard.c \
              $(SRC_DIR)/kernel/arch/i386/interrupts/interrupt.c
LINKER_SCRIPT = $(SRC_DIR)/kernel/arch/i386/boot/linker.ld
KERNEL_OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/multiboot.o $(BUILD_DIR)/display.o $(BUILD_DIR)/cmos.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/interrupt.o

# 默认目标
all: $(ISO)

# 强制重新构建
rebuild: clean all

# 确保构建目录存在
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 复制链接器脚本到构建目录
$(BUILD_DIR)/linker.ld: $(LINKER_SCRIPT) | $(BUILD_DIR)
	cp $< $@

# 编译引导加载程序
$(BOOTLOADER): $(BOOTLOADER_SRC) | $(BUILD_DIR)
	$(AS) -f bin -o $@ $<

# 编译内核对象文件
$(BUILD_DIR)/main.o: $(SRC_DIR)/kernel/kernel/core/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR)/kernel/include -c -o $@ $<

$(BUILD_DIR)/multiboot.o: $(SRC_DIR)/kernel/kernel/core/multiboot.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR)/kernel/include -c -o $@ $<

$(BUILD_DIR)/display.o: $(SRC_DIR)/kernel/drivers/display/display.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR)/kernel/include -c -o $@ $<

$(BUILD_DIR)/cmos.o: $(SRC_DIR)/kernel/drivers/cmos/cmos.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR)/kernel/include -c -o $@ $<

$(BUILD_DIR)/keyboard.o: $(SRC_DIR)/kernel/drivers/keyboard/keyboard.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR)/kernel/include -c -o $@ $<

$(BUILD_DIR)/interrupt.o: $(SRC_DIR)/kernel/arch/i386/interrupts/interrupt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR)/kernel/include -c -o $@ $<

# 链接内核
$(KERNEL): $(KERNEL_OBJS) $(BUILD_DIR)/linker.ld
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

# 创建ISO镜像
$(ISO): $(BOOTLOADER) $(KERNEL) | $(BUILD_DIR)
	@echo "Creating ISO image..."
	@mkdir -p $(BUILD_DIR)/iso/boot/grub
	@cp $(KERNEL) $(BUILD_DIR)/iso/boot/
	@echo "menuentry 'Boruix' {" > $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@echo "    multiboot /boot/kernel.bin" >> $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@echo "    boot" >> $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@echo "}" >> $(BUILD_DIR)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD_DIR)/iso/
	@echo "ISO image created: $(ISO)"

# 清理
clean:
	rm -f $(BUILD_DIR)/*.bin $(BUILD_DIR)/*.o $(BUILD_DIR)/linker.ld
	rm -rf $(BUILD_DIR)/iso/
	rm -f $(ISO)

# 深度清理（包括构建目录）
distclean: clean
	rm -rf $(BUILD_DIR)

# 运行虚拟机
run: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -boot d

# 运行虚拟机（使用软盘）
run-floppy: $(BOOTLOADER)
	qemu-system-i386 -fda $(BOOTLOADER)

# 显示项目结构
tree:
	@echo "Boruix OS 项目结构:"
	@echo "├── src/"
	@echo "│   ├── boot/"
	@echo "│   │   └── boot.asm"
	@echo "│   └── kernel/"
	@echo "│       ├── include/"
	@echo "│       │   ├── kernel/"
	@echo "│       │   │   ├── kernel.h"
	@echo "│       │   │   └── types.h"
	@echo "│       │   ├── arch/"
	@echo "│       │   │   ├── i386.h"
	@echo "│       │   │   └── interrupt.h"
	@echo "│       │   └── drivers/"
	@echo "│       │       ├── keyboard.h"
	@echo "│       │       ├── display.h"
	@echo "│       │       └── cmos.h"
	@echo "│       ├── kernel/"
	@echo "│       │   └── core/"
	@echo "│       │       └── main.c"
	@echo "│       ├── drivers/"
	@echo "│       │   ├── keyboard/"
	@echo "│       │   │   └── keyboard.c"
	@echo "│       │   ├── display/"
	@echo "│       │   │   └── display.c"
	@echo "│       │   └── cmos/"
	@echo "│       │       └── cmos.c"
	@echo "│       └── arch/"
	@echo "│           └── i386/"
	@echo "│               ├── boot/"
	@echo "│               │   ├── start.asm"
	@echo "│               │   └── linker.ld"
	@echo "│               └── interrupts/"
	@echo "│                   └── interrupt.c"
	@echo "├── build/"
	@echo "│   ├── *.bin (编译后的文件)"
	@echo "│   ├── *.o (目标文件)"
	@echo "│   └── boruix.iso"
	@echo "├── config/"
	@echo "│   └── grub.cfg"
	@echo "├── docs/"
	@echo "│   └── README.md"
	@echo "├── scripts/"
	@echo "└── Makefile"

# 显示帮助
help:
	@echo "Available targets:"
	@echo "  all        - Build the complete ISO image"
	@echo "  rebuild    - Force rebuild (clean + build)"
	@echo "  $(BUILD_DIR)/boot.bin - Build bootloader"
	@echo "  $(BUILD_DIR)/kernel.bin - Build kernel"
	@echo "  $(ISO)     - Build ISO image"
	@echo "  clean      - Clean build files"
	@echo "  distclean  - Clean build directory"
	@echo "  run        - Run in QEMU with ISO"
	@echo "  run-floppy - Run in QEMU with floppy"
	@echo "  tree       - Show project structure"
	@echo "  help       - Show this help"

.PHONY: all rebuild clean distclean run run-floppy tree help