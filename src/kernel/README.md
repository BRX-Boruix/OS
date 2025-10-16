# Boruix OS 内核目录结构

## 优化后的目录结构

```
src/kernel/
├── include/                    # 头文件目录
│   ├── kernel/                # 内核核心头文件
│   │   ├── kernel.h          # 内核主头文件
│   │   └── types.h           # 基本类型定义
│   ├── arch/                  # 架构相关头文件
│   │   ├── i386.h            # i386架构定义
│   │   └── interrupt.h       # 中断处理头文件
│   └── drivers/               # 驱动头文件
│       ├── keyboard.h         # 键盘驱动头文件
│       ├── display.h          # 显示驱动头文件
│       └── cmos.h             # CMOS时间驱动头文件
├── kernel/                     # 内核核心代码
│   └── core/                  # 核心功能
│       └── main.c             # 内核主入口
├── drivers/                    # 设备驱动
│   ├── keyboard/              # 键盘驱动
│   │   └── keyboard.c
│   ├── display/                # 显示驱动
│   │   └── display.c
│   └── cmos/                   # CMOS时间驱动
│       └── cmos.c
└── arch/                       # 架构相关代码
    └── i386/                   # i386架构
        ├── boot/              # 引导相关
        │   ├── start.asm      # 启动汇编代码
        │   └── linker.ld      # 链接器脚本
        └── interrupts/        # 中断处理
            └── interrupt.c
```

## 优化特点

### 1. 模块化分离
- **内核核心** (`kernel/core/`): 包含内核主入口和核心逻辑
- **设备驱动** (`drivers/`): 按设备类型分离，每个设备独立目录
- **架构相关** (`arch/i386/`): 架构特定代码分离

### 2. 层次化头文件组织
- **内核层** (`include/kernel/`): 核心类型和定义
- **架构层** (`include/arch/`): 架构特定定义
- **驱动层** (`include/drivers/`): 设备驱动接口

### 3. 清晰的依赖关系
- 头文件按层次组织，避免循环依赖
- 每个模块有明确的职责边界
- 便于单元测试和模块替换

### 4. 可扩展性
- 易于添加新的设备驱动
- 支持多架构扩展
- 便于添加新的内核功能模块

## 编译说明

使用更新后的Makefile进行编译：

```bash
make clean
make all
```

新的Makefile会自动处理新的目录结构和依赖关系。
