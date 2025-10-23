# Makefile for Boruix OS
# 仅支持x86_64架构，使用NASM和GCC
# 使用Limine引导加载程序

# 架构选择 (仅支持x86_64)
ARCH := x86_64

# 编译器设置
AS = nasm
CC = gcc
LD = ld
CARGO = cargo

# 架构相关设置 (仅x86_64)
ASFLAGS = -f elf64
CFLAGS = -m64 -std=c99 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -Wextra -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic -ffunction-sections -fdata-sections
LDFLAGS = -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -gc-sections -T $(BUILD_ARCH_DIR)/linker.ld
QEMU = qemu-system-x86_64
ARCH_DIR = x86_64
GRUB_MULTIBOOT = multiboot2

# Rust编译设置
RUST_TARGET = x86_64-unknown-none
RUSTFLAGS = -C code-model=kernel -C relocation-model=static

# 目录设置
SRC_DIR = src
BUILD_DIR = build
BUILD_ARCH_DIR = $(BUILD_DIR)/$(ARCH)
CONFIG_DIR = config
DOCS_DIR = docs
SCRIPTS_DIR = scripts

# 目标文件 (架构相关)
KERNEL = $(BUILD_ARCH_DIR)/boruix-kernel
ISO = $(BUILD_ARCH_DIR)/boruix-$(ARCH).iso
LIMINE_DIR = limine

# x86_64使用Limine引导加载程序

# 自动发现所有C源文件
KERNEL_COMMON_SRCS = $(wildcard $(SRC_DIR)/kernel/kernel/core/*.c) \
                     $(wildcard $(SRC_DIR)/kernel/kernel/debug/*.c) \
                     $(wildcard $(SRC_DIR)/kernel/kernel/shell/**/*.c) \
                     $(wildcard $(SRC_DIR)/kernel/kernel/shell/**/**/*.c) \
                     $(wildcard $(SRC_DIR)/kernel/drivers/cmos/*.c) \
                     $(wildcard $(SRC_DIR)/kernel/drivers/display/*.c) \
                     $(wildcard $(SRC_DIR)/kernel/drivers/keyboard/*.c) \
                     $(wildcard $(SRC_DIR)/kernel/drivers/timer/*.c) \
                     $(wildcard $(SRC_DIR)/kernel/drivers/framebuffer/*.c) \
                     $(wildcard $(SRC_DIR)/kernel/drivers/flanterm/*.c) \
                     $(wildcard $(SRC_DIR)/kernel/drivers/flanterm/flanterm_backends/*.c) \
                     $(wildcard $(SRC_DIR)/kernel/drivers/tty/*.c) \
                     $(wildcard $(SRC_DIR)/kernel/lib/*.c) \

# 架构特定的C源文件（明确指定子目录避免通配符问题）
KERNEL_ARCH_SRCS = $(wildcard $(SRC_DIR)/kernel/arch/$(ARCH_DIR)/*.c) \
                   $(wildcard $(SRC_DIR)/kernel/arch/$(ARCH_DIR)/interrupt/*.c)

# 合并所有C源文件（去重）
KERNEL_SRCS = $(sort $(KERNEL_COMMON_SRCS) $(KERNEL_ARCH_SRCS))

# 自动发现架构特定的汇编文件
KERNEL_ASM_SRCS := $(wildcard $(SRC_DIR)/kernel/arch/$(ARCH_DIR)/*.asm) \
                   $(wildcard $(SRC_DIR)/kernel/arch/$(ARCH_DIR)/boot/*.asm) \
                   $(wildcard $(SRC_DIR)/kernel/arch/$(ARCH_DIR)/interrupt/*.asm)

# 自动发现所有Rust项目（查找包含Cargo.toml的目录）
RUST_PROJECTS := $(shell find $(SRC_DIR) -name "Cargo.toml" -exec dirname {} \; 2>/dev/null)

# 生成Rust库文件列表（明确指定已知的库文件）
RUST_LIBS := $(SRC_DIR)/memory_rust/librust_memory.a

# 生成对象文件列表（加上语言后缀避免C和ASM重名）
# 格式：父文件夹-文件名-语言.o
KERNEL_C_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_ARCH_DIR)/%-c.o,$(KERNEL_SRCS))
KERNEL_ASM_OBJS = $(patsubst $(SRC_DIR)/%.asm,$(BUILD_ARCH_DIR)/%-asm.o,$(KERNEL_ASM_SRCS))
KERNEL_OBJS = $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS) $(RUST_LIBS)

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

# C源文件编译规则（新命名方式：文件名-c.o）
$(BUILD_ARCH_DIR)/%-c.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(SRC_DIR)/kernel/include -c $< -o $@

# 汇编文件编译规则（新命名方式：文件名-asm.o）
$(BUILD_ARCH_DIR)/%-asm.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# 确保Limine存在
$(LIMINE_DIR)/limine:
	@if [ ! -f $(LIMINE_DIR)/limine ]; then \
		echo "Error: Limine not found. Please clone limine repository first."; \
		echo "Run: git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1"; \
		exit 1; \
	fi

# 通用编译规则 - 自动处理所有C文件
$(BUILD_ARCH_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_ARCH_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -D__$(ARCH)__ -I$(SRC_DIR)/kernel/include -c -o $@ $<

# 通用汇编编译规则 - 自动处理所有ASM文件  
$(BUILD_ARCH_DIR)/%.o: $(SRC_DIR)/%.asm | $(BUILD_ARCH_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

# Rust项目编译规则 - 自动处理所有Rust项目
$(SRC_DIR)/%/lib%.a: $(SRC_DIR)/%/Cargo.toml $(SRC_DIR)/%/src/*.rs
	@echo "构建Rust项目: $*"
	@cd $(SRC_DIR)/$* && RUSTFLAGS="$(RUSTFLAGS)" $(CARGO) build --release --target $(RUST_TARGET)
	@cd $(SRC_DIR)/$* && cp target/$(RUST_TARGET)/release/lib*.a ./

# 通用Rust库编译规则（处理各种命名模式）
$(SRC_DIR)/memory_rust/librust_memory.a: $(SRC_DIR)/memory_rust/Cargo.toml $(wildcard $(SRC_DIR)/memory_rust/src/*.rs)
	@echo "构建Rust内存管理器..."
	@cd $(SRC_DIR)/memory_rust && RUSTFLAGS="$(RUSTFLAGS)" $(CARGO) build --release --target $(RUST_TARGET)
	@cd $(SRC_DIR)/memory_rust && cp target/$(RUST_TARGET)/release/libboruix_memory.a librust_memory.a

# 链接内核
$(KERNEL): $(KERNEL_OBJS) $(BUILD_ARCH_DIR)/linker.ld
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

# 创建ISO镜像 (使用Limine)
$(ISO): $(KERNEL) $(LIMINE_DIR)/limine | $(BUILD_ARCH_DIR)
	@echo "Creating $(ARCH) ISO image with Limine..."
	@rm -rf $(BUILD_ARCH_DIR)/iso
	@mkdir -p $(BUILD_ARCH_DIR)/iso/boot
	@cp $(KERNEL) $(BUILD_ARCH_DIR)/iso/boot/
	@mkdir -p $(BUILD_ARCH_DIR)/iso/boot/limine
	@cp limine.conf $(BUILD_ARCH_DIR)/iso/boot/limine/
	@cp $(LIMINE_DIR)/limine-bios.sys $(BUILD_ARCH_DIR)/iso/boot/limine/
	@cp $(LIMINE_DIR)/limine-bios-cd.bin $(BUILD_ARCH_DIR)/iso/boot/limine/
	@cp $(LIMINE_DIR)/limine-uefi-cd.bin $(BUILD_ARCH_DIR)/iso/boot/limine/
	@mkdir -p $(BUILD_ARCH_DIR)/iso/EFI/BOOT
	@cp $(LIMINE_DIR)/BOOTX64.EFI $(BUILD_ARCH_DIR)/iso/EFI/BOOT/
	@if [ -f $(LIMINE_DIR)/BOOTIA32.EFI ]; then \
		cp $(LIMINE_DIR)/BOOTIA32.EFI $(BUILD_ARCH_DIR)/iso/EFI/BOOT/; \
	fi
	xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(BUILD_ARCH_DIR)/iso -o $@
	$(LIMINE_DIR)/limine bios-install $@
	@echo "$(ARCH) ISO image created with Limine: $(ISO)"

# 架构特定清理
clean:
	rm -f $(BUILD_ARCH_DIR)/boruix-kernel $(BUILD_ARCH_DIR)/*.bin $(BUILD_ARCH_DIR)/*.o $(BUILD_ARCH_DIR)/linker.ld
	rm -rf $(BUILD_ARCH_DIR)/iso/
	rm -f $(ISO)
	@# 清理Rust构建产物
	@for project in $(RUST_PROJECTS); do \
		if [ -d "$$project" ]; then \
			echo "清理Rust项目: $$project"; \
			cd "$$project" && $(CARGO) clean 2>/dev/null || true; \
			rm -f "$$project"/lib*.a; \
		fi; \
	done

# 清理所有架构
clean-all:
	rm -rf $(BUILD_DIR)

# 深度清理（包括构建目录）
distclean: clean-all

# 运行虚拟机
run: $(ISO)
	$(QEMU) -cdrom $(ISO) -boot d -serial stdio -no-reboot -d cpu_reset

# 安装Limine（如果需要）
install-limine:
	@if [ ! -d $(LIMINE_DIR) ]; then \
		echo "Cloning Limine bootloader..."; \
		git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1 $(LIMINE_DIR); \
	fi
	@if [ ! -f $(LIMINE_DIR)/limine ]; then \
		echo "Building Limine..."; \
		make -C $(LIMINE_DIR); \
	fi

# 构建（仅x86_64）
build-all: all

# 仅构建Rust项目
build-rust:
	@echo "构建所有Rust项目..."
	@for project in $(RUST_PROJECTS); do \
		if [ -d "$$project" ]; then \
			echo "构建Rust项目: $$project"; \
			cd "$$project" && RUSTFLAGS="$(RUSTFLAGS)" $(CARGO) build --release --target $(RUST_TARGET); \
		fi; \
	done

# 检查Rust代码
check-rust:
	@echo "检查所有Rust项目..."
	@for project in $(RUST_PROJECTS); do \
		if [ -d "$$project" ]; then \
			echo "检查Rust项目: $$project"; \
			cd "$$project" && $(CARGO) check --target $(RUST_TARGET); \
		fi; \
	done

# 格式化Rust代码
fmt-rust:
	@echo "格式化所有Rust项目..."
	@for project in $(RUST_PROJECTS); do \
		if [ -d "$$project" ]; then \
			echo "格式化Rust项目: $$project"; \
			cd "$$project" && $(CARGO) fmt; \
		fi; \
	done

# Rust代码静态分析
clippy-rust:
	@echo "运行Clippy检查所有Rust项目..."
	@for project in $(RUST_PROJECTS); do \
		if [ -d "$$project" ]; then \
			echo "Clippy检查Rust项目: $$project"; \
			cd "$$project" && $(CARGO) clippy --target $(RUST_TARGET) -- -D warnings; \
		fi; \
	done

# 显示当前架构信息
info:
	@echo "Current architecture: $(ARCH)"
	@echo "Compiler flags: $(CFLAGS)"
	@echo "Linker flags: $(LDFLAGS)"
	@echo "QEMU: $(QEMU)"
	@echo "Build directory: $(BUILD_ARCH_DIR)"
	@echo "ISO file: $(ISO)"
	@echo ""
	@echo "Rust设置:"
	@echo "Cargo: $(CARGO)"
	@echo "Rust target: $(RUST_TARGET)"
	@echo "Rust flags: $(RUSTFLAGS)"
	@echo "发现的Rust项目: $(RUST_PROJECTS)"
	@echo "Rust库文件: $(RUST_LIBS)"

# 显示帮助
help:
	@echo "Boruix OS (x86_64 only) - Limine Bootloader + Rust支持"
	@echo ""
	@echo "主要目标:"
	@echo "  all           - 构建完整的ISO镜像（包含C和Rust代码）"
	@echo "  rebuild       - 强制重新构建"
	@echo "  clean         - 清理构建文件（包含Rust构建产物）"
	@echo "  clean-all     - 清理所有构建文件"
	@echo "  distclean     - 深度清理"
	@echo "  run           - 在QEMU中运行ISO"
	@echo ""
	@echo "Rust相关目标:"
	@echo "  build-rust    - 仅构建所有Rust项目"
	@echo "  check-rust    - 检查所有Rust代码"
	@echo "  fmt-rust      - 格式化所有Rust代码"
	@echo "  clippy-rust   - 运行Clippy静态分析"
	@echo ""
	@echo "其他目标:"
	@echo "  install-limine - 安装Limine引导加载程序"
	@echo "  info          - 显示架构和Rust信息"
	@echo "  help          - 显示此帮助"

.PHONY: all rebuild clean clean-all distclean run install-limine build-all build-rust check-rust fmt-rust clippy-rust info help