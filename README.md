# dpdk_app

一个用于学习和实验 DPDK（Data Plane Development Kit）高性能网络数据包处理的 C/C++ 练习仓库，同时包含若干独立的 C++/系统编程小示例。

## 模块一览

| 模块 | 说明 | 构建目标 |
|------|------|----------|
| 根 `main.cpp` | DPDK 单进程网卡收发骨架 | `dpdk_main` |
| `dpdk_copy/` | DPDK 多进程流量复制分发（主进程 + 两个 secondary 子进程，经 `rte_ring` 零拷贝分发） | `dpdk_copy_primary` / `dpdk_copy_subprocess` |
| `circular_buffer_test/` | Boost `circular_buffer` 循环缓冲区示例（C++14） | `CBTest` |
| `factory_test/` | 基于 `unordered_map` + `std::function` 的工厂模式示例 | `FactoryHashTest` |
| `template_test/` | C++ 模板函数示例 | `MyTemplateProgram` |
| `smart_ptr/` | C++11 智能指针（unique_ptr / shared_ptr / weak_ptr）示例 | `SmartPtrDemo` |
| `libevent_test/` | libevent2 定时器 / 信号 / HTTP 服务器（8080 端口）示例 | `libevent_demo` |
| `install/` | DPDK 环境准备与 Ubuntu 镜像同步脚本 | - |

## 构建

### 统一构建（推荐）

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -- -j
```

构建选项：

- `-DBUILD_DPDK_APPS=OFF`：跳过 DPDK 相关目标（无 DPDK 环境时使用）。
- `-DBUILD_SUBPROJECTS=OFF`：跳过 `circular_buffer_test`、`factory_test`、`template_test`、`smart_ptr`。
- `-DBUILD_LIBEVENT_DEMO=OFF`：跳过 `libevent_test`（未安装 libevent2 时会自动跳过）。

DPDK 通过 `pkg-config` 查找 `libdpdk`；libevent 需要系统安装 `libevent-dev`。

### 各子项目独立构建

每个子目录都有自己的 `CMakeLists.txt`，例如：

```bash
cd smart_ptr && mkdir -p build && cd build && cmake .. && make -j
```

`dpdk_copy/` 也可直接编译：

```bash
g++ -o main main.cpp $(pkg-config --cflags --libs libdpdk) -lpthread
g++ -o subprocess subprocess.cpp $(pkg-config --cflags --libs libdpdk) -lpthread
```

## 运行

非 DPDK 示例编译后直接运行即可，例如：

```bash
./build/smart_ptr/SmartPtrDemo
./build/libevent_test/libevent_demo   # HTTP 服务监听 0.0.0.0:8080，Ctrl+C 退出
```

DPDK 程序需要满足以下前置条件并以 root 运行：

1. 配置 HugePages：
   ```bash
   echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
   ```
2. 网卡绑定到 DPDK 驱动（`igb_uio` 或 `vfio-pci`）。
3. `dpdk_copy/` 必须先启动主进程再启动子进程，详见 `dpdk_copy/README.md`：
   ```bash
   sudo ./main --proc-type=primary -l 0-3 -n 4
   sudo ./subprocess --proc-type=secondary -l 4 -n 4 -- 1
   ```

## 注意事项

- DPDK 程序会直接操作物理网卡，请勿在生产网卡上测试。
- 数据面 fast path（收包/出队循环）中不要加 `printf`，会严重降低性能。
- 各模块之间无代码依赖，可单独取用。

## License

MIT License
