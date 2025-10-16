# Makefile for Boruix OS
# 支持i386和x86_64双架构，使用NASM和GCC
# 重新组织的项目结构

# 架构选择 (默认i386，可通过 make ARCH=x86_64 切换)
ARCH ?= i386

# 编译器设置
AS = nasm
CC = gcc
LD = ld

# 架构相关设置
ifeq ($(ARCH),x86_64)
    # x86_64 编译标志
    ASFLAGS = -f elf64
    CFLAGS = -m64 -std=c99 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -Wextra -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic
    LDFLAGS = -m elf_x86_64 -T $(BUILD_ARCH_DIR)/linker.ld
    QEMU = qemu-system-x86_64
    ARCH_DIR = x86_64
    GRUB_MULTIBOOT = multiboot2
else
    # i386 编译标志 (默认)
    ASFLAGS = -f elf32
    CFLAGS = -m32 -std=c99 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -Wextra
    LDFLAGS = -m elf_i386 -T $(BUILD_ARCH_DIR)/linker.ld
    QEMU = qemu-system-i386
    ARCH_DIR = i386
    GRUB_MULTIBOOT = multiboot
endif

# 目录设置
SRC_DIR = src
BUILD_DIR = build
BUILD_ARCH_DIR = $(BUILD_DIR)/$(ARCH)
CONFIG_DIR = config
DOCS_DIR = docs
SCRIPTS_DIR = scripts

# 目标文件 (架构相关)
BOOTLOADER = $(BUILD_ARCH_DIR)/boot.bin
KERNEL = $(BUILD_ARCH_DIR)/kernel.bin
ISO = $(BUILD_ARCH_DIR)/boruix-$(ARCH).iso

# 自动发现源文件
BOOTLOADER_SRC = $(SRC_DIR)/boot/boot.asm

# 自动发现所有C源文件
KERNEL_COMMON_SRCS = $(wildcard $(SRC_DIR)/kernel/kernel/core/*.c) \
                     $(wildcard $(SRC_DIR)/kernel/drivers/*/*.c) \
                     $(wildcard $(SRC_DIR)/kernel/memory/memory_common.c)

# 架构特定的C源文件
KERNEL_ARCH_SRCS = $(wildcard $(SRC_DIR)/kernel/arch/$(ARCH_DIR)/*/*.c) \
                   $(wildcard $(SRC_DIR)/kernel/memory/memory_$(ARCH).c)

# 合并所有C源文件
KERNEL_SRCS = $(KERNEL_COMMON_SRCS) $(KERNEL_ARCH_SRCS)

# 自动发现架构特定的汇编文件
KERNEL_ASM_SRCS = $(wildcard $(SRC_DIR)/kernel/arch/$(ARCH_DIR)/boot/*.asm)

# 生成对象文件列表
KERNEL_C_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_ARCH_DIR)/%.o,$(KERNEL_SRCS))
KERNEL_ASM_OBJS = $(patsubst $(SRC_DIR)/%.asm,$(BUILD_ARCH_DIR)/%.o,$(KERNEL_ASM_SRCS))
KERNEL_OBJS = $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS)

LINKER_SCRIPT = $(SRC_DIR)/kernel/arch/$(ARCH_DIR)/boot/linker.ld

# 默认目标
all: $(ISO)

# 强制重新构建
rebuild: clean all

# 确保构建目录存在
$(BUILD_ARCH_DIR):
	mkdir -p $(BUILD_ARCH_DIR)

# 复制链接器脚本到构建目录
$(BUILD_ARCH_DIR)/linker.ld: $(LINKER_SCRIPT) | $(BUILD_ARCH_DIR)
	cp $< $@

# 编译引导加载程序
$(BOOTLOADER): $(BOOTLOADER_SRC) | $(BUILD_ARCH_DIR)
	$(AS) -f bin -o $@ $<

# 通用编译规则 - 自动处理所有C文件
$(BUILD_ARCH_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_ARCH_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -D__$(ARCH)__ -I$(SRC_DIR)/kernel/include -c -o $@ $<

# 通用汇编编译规则 - 自动处理所有ASM文件  
$(BUILD_ARCH_DIR)/%.o: $(SRC_DIR)/%.asm | $(BUILD_ARCH_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

# 链接内核
$(KERNEL): $(KERNEL_OBJS) $(BUILD_ARCH_DIR)/linker.ld
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

# 创建ISO镜像
$(ISO): $(BOOTLOADER) $(KERNEL) | $(BUILD_ARCH_DIR)
	@echo "Creating $(ARCH) ISO image..."
	@mkdir -p $(BUILD_ARCH_DIR)/iso/boot/grub
	@cp $(KERNEL) $(BUILD_ARCH_DIR)/iso/boot/
	@echo "menuentry 'Boruix OS ($(ARCH))' {" > $(BUILD_ARCH_DIR)/iso/boot/grub/grub.cfg
ifeq ($(ARCH),x86_64)
	@echo "    multiboot /boot/kernel.bin" >> $(BUILD_ARCH_DIR)/iso/boot/grub/grub.cfg
else
	@echo "    $(GRUB_MULTIBOOT) /boot/kernel.bin" >> $(BUILD_ARCH_DIR)/iso/boot/grub/grub.cfg
endif
	@echo "    boot" >> $(BUILD_ARCH_DIR)/iso/boot/grub/grub.cfg
	@echo "}" >> $(BUILD_ARCH_DIR)/iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(BUILD_ARCH_DIR)/iso/
	@echo "$(ARCH) ISO image created: $(ISO)"

# 架构特定清理
clean:
	rm -f $(BUILD_ARCH_DIR)/*.bin $(BUILD_ARCH_DIR)/*.o $(BUILD_ARCH_DIR)/linker.ld
	rm -rf $(BUILD_ARCH_DIR)/iso/
	rm -f $(ISO)

# 清理所有架构
clean-all:
	rm -rf $(BUILD_DIR)

# 深度清理（包括构建目录）
distclean: clean-all

# 运行虚拟机
run: $(ISO)
	$(QEMU) -cdrom $(ISO) -boot d

# 运行虚拟机（使用软盘）
run-floppy: $(BOOTLOADER)
	$(QEMU) -fda $(BOOTLOADER)

# 构建所有架构
build-all:
	@echo "Building i386 version..."
	$(MAKE) ARCH=i386 all
	@echo "Building x86_64 version..."
	$(MAKE) ARCH=x86_64 all

# 显示当前架构信息
info:
	@echo "Current architecture: $(ARCH)"
	@echo "Compiler flags: $(CFLAGS)"
	@echo "Linker flags: $(LDFLAGS)"
	@echo "QEMU: $(QEMU)"
	@echo "Build directory: $(BUILD_ARCH_DIR)"
	@echo "ISO file: $(ISO)"

# 显示帮助
help:
	@echo "Boruix OS"
	@echo ""
	@echo "架构选择:"
	@echo "  make ARCH=i386 [target]   - 构建32位版本 (默认)"
	@echo "  make ARCH=x86_64 [target] - 构建64位版本"
	@echo ""
	@echo "可用目标:"
	@echo "  all        - 构建完整的ISO镜像"
	@echo "  rebuild    - 强制重新构建"
	@echo "  build-all  - 构建所有架构版本"
	@echo "  clean      - 清理当前架构构建文件"
	@echo "  clean-all  - 清理所有架构构建文件"
	@echo "  distclean  - 深度清理"
	@echo "  run        - 在QEMU中运行ISO"
	@echo "  run-floppy - 在QEMU中运行软盘"
	@echo "  info       - 显示当前架构信息"
	@echo "  help       - 显示此帮助"

.PHONY: all rebuild clean clean-all distclean run run-floppy build-all info help