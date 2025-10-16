Boruix OS 采用 `computer:/` 文件系统设计，将硬件抽象化为文件系统结构，提供直观的硬件访问和控制接口。本设计结合了 Unix-like 系统的灵活性和现代操作系统的用户友好性。

- **硬件即文件**：将所有硬件组件抽象为文件系统接口
- **实时交互**：通过文件读写直接与硬件交互
- **状态透明**：硬件状态通过文件系统完全暴露
- **多层次访问**：提供不同复杂度的访问路径
- **子系统化**：按功能将硬件组织成逻辑子系统
- **层次清晰**：每个子系统内部保持清晰的层次结构
- **扩展性强**：易于添加新的硬件类型和设备
- **用户中心**：提供直观易懂的访问方式

## 文件系统结构

### 完整目录树

```
computer:/
├── cpu/                    # 处理器子系统
│   ├── core0/             # CPU核心0
│   │   ├── freq           # 频率
│   │   ├── temp           # 温度
│   │   └── usage          # 使用率
│   ├── core1/             # CPU核心1
│   ├── info               # CPU总体信息
│   ├── cache/             # 缓存信息
│   │   ├── l1             # L1缓存
│   │   ├── l2             # L2缓存
│   │   └── l3             # L3缓存
│   └── governor           # 频率调节器
├── mem/                    # 内存子系统
│   ├── physical/          # 物理内存
│   │   ├── total          # 总容量
│   │   ├── free           # 可用容量
│   │   ├── used           # 已用容量
│   │   └── banks/         # 内存条信息
│   ├── virtual/           # 虚拟内存
│   │   ├── pages          # 页面统计
│   │   └── swap           # 交换分区
│   └── cache/             # 内存缓存
├── disk/                   # 快速访问分区内容（日常使用）
│   ├── a/                 # 第1个分区
│   ├── b/                 # 第2个分区
│   ├── c/                 # 第3个分区（通常是系统盘）
│   │   ├── SYSTEM/        # 系统文件
│   │   ├── programs/      # 程序文件
│   │   └── users/         # 用户文件
│   ├── d/                 # 第4个分区（数据盘）
│   │   ├── documents/     # 文档
│   │   └── media/         # 媒体文件
│   ├── ...                # 更多分区 (e, f, g, ..., z)
│   ├── aa/                # 第27个分区（26字母用完后）
│   ├── ab/                # 第28个分区
│   ├── ...                # 更多分区 (ac, ad, ..., az, ba, bb, ..., zz)
│   └── aaa/               # 第703个分区（两字母用完后）
├── mount/                  # 灵活挂载系统
│   ├── auto/              # 自动挂载区域
│   │   ├── system -> ../disk/c     # 符号链接到disk/c
│   │   ├── data -> ../disk/d       # 符号链接到disk/d
│   │   └── swap           # 交换分区
│   ├── user/              # 用户挂载区域
│   │   ├── backup/        # 用户自定义挂载点
│   │   ├── project_disk/  # 项目专用磁盘
│   │   └── shared_folder/ # 共享文件夹
│   ├── removable/         # 可移动设备自动挂载
│   │   ├── usb_001/       # USB设备（自动命名）
│   │   ├── usb_002/
│   │   └── cdrom/
│   └── network/           # 网络挂载
│       ├── ftp_server/
│       ├── nfs_share/
│       └── smb_share/
├── storage/                # 存储子系统（硬件管理层）
│   ├── disk0/             # 第一块硬盘
│   │   ├── info           # 磁盘信息
│   │   ├── health         # 健康状态
│   │   ├── partitions/    # 分区列表
│   │   │   ├── part1/     # 分区1
│   │   │   │   ├── mounted_at -> /disk/c  # 指向实际挂载点
│   │   │   │   ├── size   # 大小
│   │   │   │   ├── free   # 可用空间
│   │   │   │   └── format # 文件系统格式
│   │   │   └── part2/     # 分区2
│   │   │       └── mounted_at -> /disk/d
│   │   └── smart/         # SMART信息
│   ├── disk1/             # 第二块硬盘
│   ├── optical/           # 光驱
│   │   ├── cd0/
│   │   └── dvd0/
│   └── removable/         # 可移动存储
│       ├── usb0/
│       │   └── mounted_at -> /mount/removable/usb_001
│       ├── usb1/
│       └── floppy/
├── display/                # 显示子系统
│   ├── screen0/           # 主显示器
│   │   ├── resolution     # 分辨率
│   │   ├── refresh        # 刷新率
│   │   ├── brightness     # 亮度
│   │   └── mode           # 显示模式
│   ├── screen1/           # 副显示器
│   └── gpu/               # 显卡信息
│       ├── info           # 显卡规格
│       ├── temp           # 温度
│       └── memory         # 显存
├── audio/                  # 音频子系统
│   ├── playback/          # 播放设备
│   │   ├── speakers/
│   │   │   ├── volume     # 音量
│   │   │   ├── mute       # 静音
│   │   │   └── channels   # 声道
│   │   └── headphones/
│   ├── capture/           # 录音设备
│   │   └── microphone/
│   └── mixer/             # 混音器
├── network/                # 网络子系统
│   ├── ethernet/          # 有线网络
│   │   └── eth0/
│   │       ├── ip         # IP地址
│   │       ├── mac        # MAC地址
│   │       ├── status     # 连接状态
│   │       ├── speed      # 连接速度
│   │       └── stats/     # 统计信息
│   ├── wireless/          # 无线网络
│   │   └── wlan0/
│   │       ├── ssid       # 网络名称
│   │       ├── signal     # 信号强度
│   │       └── security   # 安全类型
│   ├── bluetooth/         # 蓝牙
│   │   ├── status         # 蓝牙状态
│   │   └── devices/       # 配对设备
│   └── loopback/          # 回环接口
├── input/                  # 输入设备子系统
│   ├── keyboard/          # 键盘
│   │   ├── layout         # 键盘布局
│   │   └── repeat         # 重复设置
│   ├── mouse/             # 鼠标
│   │   ├── sensitivity    # 灵敏度
│   │   └── buttons        # 按键设置
│   ├── touchpad/          # 触摸板
│   └── gamepad/           # 游戏手柄
├── terminal/               # 终端子系统
│   ├── console/           # 控制台
│   │   └── tty0           # 主控制台
│   ├── virtual/           # 虚拟终端
│   │   ├── tty1
│   │   ├── tty2
│   │   └── tty3
│   └── serial/            # 串口终端
│       ├── ttyS0
│       └── ttyS1
├── power/                  # 电源管理子系统
│   ├── battery/           # 电池（笔记本）
│   │   ├── charge         # 电量
│   │   ├── status         # 充电状态
│   │   └── health         # 电池健康
│   ├── thermal/           # 热管理
│   │   ├── zones/         # 温度区域
│   │   └── cooling/       # 散热设备
│   └── acpi/              # ACPI电源管理
├── SYSTEM/                 # 系统控制中心
│   ├── control/           # 系统控制
│   │   ├── reboot         # 重启
│   │   ├── shutdown       # 关机
│   │   ├── suspend        # 挂起
│   │   └── hibernate      # 休眠
│   ├── info/              # 系统信息
│   │   ├── version        # 系统版本
│   │   ├── uptime         # 运行时间
│   │   ├── hostname       # 主机名
│   │   └── kernel         # 内核信息
│   ├── services/          # 系统服务
│   │   ├── running        # 运行中的服务
│   │   ├── stopped        # 已停止的服务
│   │   └── failed         # 失败的服务
│   └── logs/              # 系统日志
│       ├── kernel         # 内核日志
│       ├── system         # 系统日志
│       └── error          # 错误日志
├── runtime/                # 运行时环境
│   ├── processes/         # 进程管理
│   │   ├── list           # 进程列表
│   │   ├── tree           # 进程树
│   │   └── stats          # 进程统计
│   ├── modules/           # 内核模块
│   │   ├── loaded         # 已加载模块
│   │   └── available      # 可用模块
│   └── interrupts/        # 中断信息
├── users/                  # 用户空间
├── programs/               # 程序目录
├── library/                # 系统库
└── workspace/              # 用户工作空间
```


## 实现计划

### 第一阶段：基础框架

#### 1.1 虚拟文件系统核心
- [ ] 实现 `computer:/` 根文件系统挂载点
- [ ] 创建虚拟文件系统驱动 (VFS Driver)
- [ ] 实现基础的文件操作接口 (open, read, write, close)
- [ ] 支持目录遍历和文件属性查询
- [ ] 实现文件系统注册和卸载机制

#### 1.2 设备抽象层
- [ ] 定义统一的设备接口规范
- [ ] 实现设备注册和发现机制
- [ ] 创建设备状态管理器
- [ ] 实现设备事件通知系统
- [ ] 建立设备到文件路径的映射机制

#### 1.3 状态文件支持
- [ ] 实现只读状态文件 (如 CPU 温度、内存使用率)
- [ ] 实现可写控制文件 (如亮度调节、音量控制)
- [ ] 支持实时数据更新机制
- [ ] 实现文件内容格式化 (JSON/纯文本)
- [ ] 添加文件访问权限控制

#### 1.4 基础目录结构
- [ ] 创建核心子系统目录 (`cpu/`, `mem/`, `disk/`, `SYSTEM/`)
- [ ] 实现动态目录生成 (根据硬件配置)
- [ ] 支持符号链接的基础框架
- [ ] 实现目录权限和可见性控制

#### 1.5 开发工具和测试
- [ ] 创建文件系统调试工具
- [ ] 实现基础的单元测试框架
- [ ] 建立模拟硬件环境用于测试
- [ ] 创建性能监控和日志系统
- [ ] 编写开发文档和API参考

### 第二阶段：存储系统
- [ ] 实现 `computer:/disk/` 快速访问
- [ ] 实现 `computer:/mount/` 挂载管理
- [ ] 实现 `computer:/storage/` 硬件管理

### 第三阶段：硬件集成
- [ ] CPU、内存、网络等子系统
- [ ] 设备热插拔支持
- [ ] 实时状态更新机制

### 第四阶段：高级功能
- [ ] 符号链接支持
- [ ] 权限管理系统
- [ ] 网络文件系统支持

### 第五阶段：优化完善
- [ ] 性能优化
- [ ] 错误处理完善
- [ ] 用户界面工具

---
