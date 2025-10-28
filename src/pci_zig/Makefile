# PCI驱动 - Zig Makefile

ZIG = zig
BUILD_DIR = zig-out
TARGET_OBJ = $(BUILD_DIR)/pci.o
TARGET_LIB = $(BUILD_DIR)/libpci.a

# Zig编译标志
ZIG_FLAGS = -target x86_64-freestanding-none \
            -O ReleaseSmall \
            -fno-stack-check \
            -mcmodel=large \
            -fno-lto

.PHONY: all clean install check fmt

all: $(TARGET_LIB)

$(TARGET_OBJ): src/pci.zig
	@echo "Building PCI driver object with Zig..."
	@mkdir -p $(BUILD_DIR)
	@$(ZIG) build-obj src/pci.zig $(ZIG_FLAGS) -femit-bin=$(TARGET_OBJ)

$(TARGET_LIB): $(TARGET_OBJ)
	@echo "Creating PCI library archive..."
	@ar rcs $(TARGET_LIB) $(TARGET_OBJ)
	@echo "PCI driver built: $(TARGET_LIB)"

clean:
	@echo "Cleaning PCI driver build..."
	@rm -rf zig-out zig-cache .zig-cache
	@rm -f *.o *.a *.ll
	@echo "Clean complete"

install: $(TARGET_LIB)
	@echo "Installing PCI driver..."
	@mkdir -p ../../build/lib
	@cp $(TARGET_LIB) ../../build/lib/
	@echo "Install complete"

# 检查Zig代码
check:
	@$(ZIG) build-lib src/pci.zig $(ZIG_FLAGS) -femit-llvm-ir=$(BUILD_DIR)/pci.ll

# 格式化代码
fmt:
	@$(ZIG) fmt src/

