# Boruix OS Rust内存管理器 Makefile
# x86_64架构内存管理系统构建脚本

# 编译器和工具
CARGO = cargo
CC = gcc
AR = ar
OBJCOPY = objcopy

# 目标架构
TARGET = x86_64-unknown-none

# 构建模式
BUILD_MODE ?= release
ifeq ($(BUILD_MODE), debug)
    CARGO_FLAGS = 
    TARGET_DIR = target/$(TARGET)/debug
else
    CARGO_FLAGS = --release
    TARGET_DIR = target/$(TARGET)/release
endif

# 输出文件
RUST_LIB = $(TARGET_DIR)/libboruix_memory.a
OUTPUT_LIB = librust_memory.a
HEADER_FILE = include/rust_memory.h

# 源文件
RUST_SOURCES = $(wildcard src/*.rs)
C_SOURCES = $(wildcard c_wrapper/*.c)

# 目标文件
C_OBJECTS = $(C_SOURCES:.c=.o)

# 编译标志
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2 -fno-stack-protector \
         -fno-builtin -nostdlib -nostdinc -ffreestanding \
         -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
         -mcmodel=kernel -I./include

# Rust编译标志
RUSTFLAGS = -C code-model=kernel -C relocation-model=static

# 默认目标
.PHONY: all
all: $(OUTPUT_LIB)

# 构建Rust库
$(RUST_LIB): $(RUST_SOURCES) Cargo.toml
	@echo "构建Rust内存管理器..."
	RUSTFLAGS="$(RUSTFLAGS)" $(CARGO) build $(CARGO_FLAGS) --target $(TARGET)

# 构建C包装器（如果有）
%.o: %.c
	@echo "编译C包装器: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# 创建最终库文件
$(OUTPUT_LIB): $(RUST_LIB) $(C_OBJECTS)
	@echo "创建最终库文件..."
	cp $(RUST_LIB) $(OUTPUT_LIB)
	@if [ -n "$(C_OBJECTS)" ]; then \
		$(AR) rcs $(OUTPUT_LIB) $(C_OBJECTS); \
	fi

# 安装到内核目录
.PHONY: install
install: $(OUTPUT_LIB)
	@echo "安装到内核目录..."
	@mkdir -p ../kernel/lib
	@mkdir -p ../kernel/include/rust
	cp $(OUTPUT_LIB) ../kernel/lib/
	cp $(HEADER_FILE) ../kernel/include/rust/

# 清理
.PHONY: clean
clean:
	@echo "清理构建文件..."
	$(CARGO) clean
	rm -f $(C_OBJECTS)
	rm -f $(OUTPUT_LIB)

# 深度清理
.PHONY: distclean
distclean: clean
	rm -rf target/
	rm -f Cargo.lock

# 检查代码
.PHONY: check
check:
	@echo "检查Rust代码..."
	$(CARGO) check --target $(TARGET)

# 运行测试（在宿主机上）
.PHONY: test
test:
	@echo "运行测试..."
	$(CARGO) test

# 格式化代码
.PHONY: fmt
fmt:
	@echo "格式化代码..."
	$(CARGO) fmt

# 代码检查
.PHONY: clippy
clippy:
	@echo "运行Clippy检查..."
	$(CARGO) clippy --target $(TARGET) -- -D warnings

# 生成文档
.PHONY: doc
doc:
	@echo "生成文档..."
	$(CARGO) doc --target $(TARGET) --no-deps

# 显示库信息
.PHONY: info
info: $(RUST_LIB)
	@echo "=== Rust内存管理器信息 ==="
	@echo "目标架构: $(TARGET)"
	@echo "构建模式: $(BUILD_MODE)"
	@echo "库文件: $(RUST_LIB)"
	@echo "输出库: $(OUTPUT_LIB)"
	@echo "头文件: $(HEADER_FILE)"
	@if [ -f $(RUST_LIB) ]; then \
		echo "库大小: $$(du -h $(RUST_LIB) | cut -f1)"; \
		echo "符号信息:"; \
		nm $(RUST_LIB) | grep -E '^[0-9a-f]+ [TtDd] ' | head -10; \
	fi

# 调试构建
.PHONY: debug
debug:
	$(MAKE) BUILD_MODE=debug

# 发布构建
.PHONY: release  
release:
	$(MAKE) BUILD_MODE=release

# 帮助信息
.PHONY: help
help:
	@echo "Boruix OS Rust内存管理器构建系统"
	@echo ""
	@echo "可用目标:"
	@echo "  all       - 构建库文件（默认）"
	@echo "  debug     - 调试模式构建"
	@echo "  release   - 发布模式构建"
	@echo "  install   - 安装到内核目录"
	@echo "  clean     - 清理构建文件"
	@echo "  distclean - 深度清理"
	@echo "  check     - 检查代码"
	@echo "  test      - 运行测试"
	@echo "  fmt       - 格式化代码"
	@echo "  clippy    - 运行Clippy检查"
	@echo "  doc       - 生成文档"
	@echo "  info      - 显示库信息"
	@echo "  help      - 显示此帮助"
	@echo ""
	@echo "环境变量:"
	@echo "  BUILD_MODE - 构建模式 (debug|release，默认release)"
