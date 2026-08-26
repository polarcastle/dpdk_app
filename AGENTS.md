# AGENTS.md

本文档为 AI 编码助手提供本代码库的工作指导。阅读本文档前，默认读者对本项目一无所知。

## 项目概述

`dpdk_app` 是一个用于学习和实验 DPDK（Data Plane Development Kit）高性能网络数据包处理的 C/C++ 练习仓库。项目采用 CMake 作为统一构建系统，根目录下包含多个独立的小模块/示例程序，各模块既可以作为根项目的子项目一起构建，也可以进入各自的 `build/` 目录单独构建。

当前仓库没有单一的「主应用」概念，而是由以下几个并列的实验性子项目组成：

- **根目录 `main.cpp`**：一个基础的 DPDK 网卡收发程序骨架。
- **`dpdk_copy/`**：DPDK 多进程流量复制分发示例（主进程 + 多个子进程通过 `rte_ring` 共享数据包）。
- **`circular_buffer_test/`**：基于 Boost 的 `circular_buffer` 循环缓冲区使用示例。
- **`factory_test/`**：基于 `std::unordered_map` + `std::function` 实现的工厂模式示例。
- **`template_test/`**：C++ 模板函数最大值示例。
- **`libevent_test/`**：基于 libevent2 的定时器、信号与 HTTP 服务器示例。
- **`smart_ptr/`**：C++11 智能指针（`unique_ptr`/`shared_ptr`/`weak_ptr`）示例。
- **`install/`**：与 DPDK 环境准备和 Ubuntu 镜像同步相关的辅助脚本。

## 目录结构

```
.
├── CMakeLists.txt              # 根 CMake 配置
├── main.cpp                    # 根 DPDK 网卡收发骨架
├── README.md                   # 项目说明（中文）
├── AGENTS.md                   # 本文件
├── .gitignore                  # Git 忽略规则
├── .vscode/settings.json       # VS Code 配置（cmake.sourceDirectory 指向 circular_buffer_test）
├── build/                      # 根构建目录（已生成，包含 Ninja 构建产物）
├── circular_buffer_test/       # Boost circular_buffer 示例
│   ├── CMakeLists.txt
│   └── cb_main.cpp
├── dpdk_copy/                  # DPDK 多进程流量复制
│   ├── AGENTS.md               # 该子目录的详细运行说明
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── main.cpp
│   └── subprocess.cpp
├── factory_test/               # 工厂模式示例
│   ├── CMakeLists.txt
│   └── FactoryTest.cpp
├── install/                    # 环境准备脚本
│   ├── deb_sync.sh
│   ├── install_deb.sh
│   └── install_dpdk_req.sh
├── libevent_test/              # libevent2 示例
│   ├── CMakeLists.txt
│   ├── libevent_demo           # 预编译二进制（已被 .gitignore 忽略）
│   └── main.cpp
├── smart_ptr/                  # C++11 智能指针示例
│   ├── CMakeLists.txt
│   └── smart_ptr_main.cpp
└── template_test/              # C++ 模板示例
    ├── CMakeLists.txt
    └── TemplateClassTest.cpp
```

## 技术栈

- **语言**：C / C++（各子项目分别使用 C++11 或 C++14）。
- **构建系统**：CMake（根项目统一构建，各子项目也可独立构建）。
- **核心依赖**：
  - DPDK（`libdpdk`，通过 `pkg-config` 查找），用于根 `main.cpp` 与 `dpdk_copy/`。
  - Boost（`boost/circular_buffer.hpp`），用于 `circular_buffer_test/`。
  - libevent2（`event2/event.h`、`event2/http.h`、`event2/buffer.h`），用于 `libevent_test/`。
- **编译器/工具链**：GCC/G++、CMake >= 3.10、`pkg-config`、Ninja（可选）。
- **运行环境**：Linux；DPDK 相关程序需要 root 权限、HugePages 以及绑定到 DPDK 驱动的网卡。

## 构建命令

### 根项目统一构建（推荐）

```bash
# 1. 配置（使用 Ninja）
cmake -S /home/lxy/dpdk_app -B /home/lxy/dpdk_app/build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 2. 编译
cmake --build /home/lxy/dpdk_app/build -- -j$(nproc)
```

- 根 `CMakeLists.txt` 会尝试通过 `pkg-config` 查找 `libdpdk`。
- 可通过以下选项控制构建范围：
  - `-DBUILD_DPDK_APPS=OFF`：跳过 DPDK 可执行文件（`dpdk_main`、`dpdk_copy_primary`、`dpdk_copy_subprocess`）。
  - `-DBUILD_SUBPROJECTS=OFF`：跳过 `circular_buffer_test`、`factory_test`、`template_test`、`smart_ptr` 子目录。
  - `-DBUILD_LIBEVENT_DEMO=OFF`：跳过 `libevent_test`（未安装 libevent2 时会自动跳过）。
- 生成的目标名称：
  - `dpdk_main`（来自根 `main.cpp`）
  - `dpdk_copy_primary`、`dpdk_copy_subprocess`（来自 `dpdk_copy/`）
  - `CBTest`（来自 `circular_buffer_test/`）
  - `FactoryHashTest`（来自 `factory_test/`）
  - `MyTemplateProgram`（来自 `template_test/`）
  - `SmartPtrDemo`（来自 `smart_ptr/`）
  - `libevent_demo`（来自 `libevent_test/`）

### 各子项目独立构建

各子目录均含有自己的 `CMakeLists.txt`，可单独进入子目录的 `build/` 执行：

```bash
cd circular_buffer_test/build && cmake .. && make -j$(nproc)
cd factory_test/build        && cmake .. && make -j$(nproc)
cd template_test/build       && cmake .. && make -j$(nproc)
cd smart_ptr/build           && cmake .. && make -j$(nproc)
cd libevent_test/build       && cmake .. && make -j$(nproc)
cd dpdk_copy/build           && cmake .. && make -j$(nproc)
```

> `dpdk_copy/` 子目录的 `CMakeLists.txt` 将生成 `main` 与 `subprocess` 两个可执行文件。

### 直接编译（替代方案）

对于 `dpdk_copy/`，也可以不使用 CMake，直接调用 `pkg-config` 编译：

```bash
g++ -o main main.cpp $(pkg-config --cflags --libs libdpdk) -lpthread
g++ -o subprocess subprocess.cpp $(pkg-config --cflags --libs libdpdk) -lpthread
```

### libevent_test 编译说明

`libevent_test/` 已接入根 CMake（目标 `libevent_demo`），也保留了手动编译方式：

```bash
cd libevent_test
g++ -o libevent_demo main.cpp -levent -levent_pthreads -std=c++11
```

## 运行架构

### 根 `main.cpp`

一个单进程 DPDK 网卡收发骨架：

1. 调用 `rte_eal_init()` 初始化 EAL（单次调用，保存返回值并更新 `argc/argv`）。
2. 创建 `rte_mempool`。
3. 配置并启动网卡端口（RX/TX 队列各 1 个，含队列 setup）。
4. 在循环中接收数据包并立刻释放；发送逻辑留空，由使用者按需填充。

### `dpdk_copy/`

高性能多进程流量复制分发系统：

- **主进程 `main`**：初始化 EAL、创建共享 `rte_mempool`、创建两个 `rte_ring`、配置网卡，然后在 RX 循环中批量收包，对每个包调用 `rte_pktmbuf_clone()` 克隆两份，通过 `rte_ring_enqueue_burst()` 批量入队到 `ring_1` 和 `ring_2`，最后 `rte_pktmbuf_free_bulk()` 批量释放原始包；支持 SIGINT/SIGTERM 优雅退出并清理资源。
- **子进程 `subprocess`**：以 secondary 方式初始化 EAL，通过名称查找共享 `mempool` 与指定 `ring`，`rte_ring_dequeue_burst()` 批量出队处理并批量释放；fast path 中无 printf。

关键常量（`main.cpp` 与 `subprocess.cpp` 必须完全一致）：

```cpp
#define MEMPOOL_NAME "shared_mbuf_pool"
#define RING_1_NAME  "ring_1"
#define RING_2_NAME  "ring_2"
```

更详细的运行步骤、HugePages 配置、网卡绑定、启动顺序、内存管理规则等，请参阅 `dpdk_copy/AGENTS.md` 与 `dpdk_copy/README.md`。

### `libevent_test/`

libevent2 Reactor 模式演示程序：

- 创建 `event_base`。
- 配置每秒触发的定时器事件。
- 配置 `SIGINT` 信号处理，优雅退出事件循环。
- 启动 `0.0.0.0:8080` HTTP 服务器，统一回调返回固定 HTML 页面。

## 代码组织

- **根项目**通过 `CMakeLists.txt` 统一聚合所有子模块。
- **每个子模块**都是自包含的：拥有独立的 `CMakeLists.txt`、单/少量源文件、独立构建产物。
- 没有共享库或公共头文件；各模块之间没有代码依赖。

## 代码风格指南

- **C++ 标准**：根项目默认使用 C++11，可通过 `-DCMAKE_CXX_STANDARD=...` 覆盖；各子项目强制各自标准：
  - `circular_buffer_test`：C++14
  - `factory_test`、`template_test`、`dpdk_copy`、`smart_ptr`、`libevent_test`：C++11
- **命名**：
  - DPDK API 使用 `rte_` 前缀、`snake_case` 命名。
  - 项目内部类名使用 `PascalCase`（如 `Factory`、`Product`），函数/变量使用 `snake_case` 或 `camelCase`，各子项目略有差异，没有统一强制规范。
- **注释与文档**：代码注释、子项目 `README.md`、`AGENTS.md` 主要以中文撰写。新增文档应继续使用中文。
- **EAL 参数分隔**：DPDK 程序启动时 EAL 参数放在 `--` 之前，应用参数放在 `--` 之后，例如：
  ```bash
  sudo ./main --proc-type=primary -l 0-3 -n 4
  sudo ./subprocess --proc-type=secondary -l 4 -n 4 -- 1
  ```

## 测试说明

- 当前仓库**没有自动化测试框架**（如 GoogleTest、Catch2）。
- 各子项目本质上都是可独立运行的示例/小程序，测试方式为编译后手动运行。
- 非 DPDK 示例如 `CBTest`、`FactoryHashTest`、`MyTemplateProgram`、`SmartPtrDemo`、`libevent_demo` 可直接运行验证输出。
- DPDK 示例（根 `main.cpp`、`dpdk_copy/`）需要满足以下前置条件才能运行：
  1. 系统已配置足够 HugePages：
     ```bash
     echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
     ```
  2. 网卡已绑定到 DPDK 驱动（如 `igb_uio` 或 `vfio-pci`）。
  3. 以 root 权限运行。

## 部署流程

项目没有正式的安装/部署流水线。现有 `install/` 目录包含：

- `install_dpdk_req.sh`：安装编译 DPDK 所需的系统包（`build-essential`、`meson`、`python3-pyelftools`）。
- `install_deb.sh`：安装 `debmirror` 并创建本地镜像目录 `/opt/ubuntu_22.04_mirror`。
- `deb_sync.sh`：使用 `debmirror` 同步 Ubuntu 22.04（jammy）镜像到本地（源服务器为清华大学镜像）。

这些脚本主要用于环境准备，不是项目本身的部署脚本。

## 安全注意事项

1. **必须以 root 权限运行 DPDK 程序**，以访问 HugePages 和网卡硬件资源。
2. **DPDK 程序会直接操作物理网卡**，运行前务必确认目标网卡已正确绑定，避免影响生产网络。
3. **`dpdk_copy/` 的多进程共享内存机制要求主进程先启动**；子进程在共享对象不存在时会失败。
4. **内存管理**：主进程负责创建并释放原始 mbuf，子进程负责释放从 ring 中出队的 mbuf；克隆包使用 `rte_pktmbuf_clone()` 共享数据缓冲区，避免重复拷贝，但需要确保各端正确释放，防止内存泄漏或重复释放。
5. **不要在 `dpdk_copy/main.cpp` 的 RX 循环中添加 `printf`**，会严重降低高性能数据面性能。
6. `libevent_test/` 的 HTTP 服务器绑定在 `0.0.0.0:8080`，在公共网络中运行可能暴露服务，仅建议在受控环境测试。

## 项目特定注意事项

- `.vscode/settings.json` 中 `cmake.sourceDirectory` 指向 `circular_buffer_test`，因此 VS Code CMake 插件默认只会配置该子目录；若要使用根 CMake，请手动切换 `cmake.sourceDirectory` 到 `/home/lxy/dpdk_app`。
- `.gitignore` 已忽略各 `build/` 目录与 `libevent_test/libevent_demo` 预编译二进制，新增构建产物不应提交。
- `dpdk_copy/AGENTS.md` 包含该子模块更详细的运行与内存管理说明，修改 `dpdk_copy/main.cpp` 或 `subprocess.cpp` 时应同步核对该文档。
