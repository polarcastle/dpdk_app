# AGENTS.md

本文档为 AI 助手提供本代码库的工作指导。

## 构建命令

```bash
# CMake 构建（推荐）
mkdir build && cd build
cmake ..
make -j$(nproc)

# 直接编译（替代方案）
g++ -o main main.cpp $(pkg-config --cflags --libs libdpdk) -lpthread
g++ -o subprocess subprocess.cpp $(pkg-config --cflags --libs libdpdk) -lpthread
```

## 运行时关键要求

1. **运行前必须配置 HugePages：**
   ```bash
   echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
   ```

2. **网卡必须绑定到 DPDK 驱动：**
   ```bash
   sudo modprobe uio
   sudo insmod /path/to/dpdk/build/kernel/linux/igb_uio/igb_uio.ko
   sudo /path/to/dpdk/usertools/dpdk-devbind.py --bind=igb_uio 0000:xx:00.0
   ```

## 进程启动顺序（顺序很重要！）

**第一步：先启动主进程**
```bash
sudo ./main --proc-type=primary -l 0-3 -n 4
```

**第二步：再启动子进程**
```bash
# 终端 2 - 连接 ring_1
sudo ./subprocess --proc-type=secondary -l 4 -n 4 -- 1

# 终端 3 - 连接 ring_2
sudo ./subprocess --proc-type=secondary -l 5 -n 4 -- 2
```

注意：`--` 之后的最后一个参数 `1` 或 `2` 选择要连接的 ring（EAL 参数必须放在 `--` 之前）。

## 关键共享内存常量

以下宏在 [`main.cpp`](main.cpp:10) 和 [`subprocess.cpp`](subprocess.cpp:7) 中**必须完全匹配**：

```cpp
#define MEMPOOL_NAME "shared_mbuf_pool"
#define RING_1_NAME "ring_1"
#define RING_2_NAME "ring_2"
```

子进程通过 [`rte_mempool_lookup()`](subprocess.cpp:16) 和 [`rte_ring_lookup()`](subprocess.cpp:18) 按名称查找这些共享对象。

## 内存管理规则

- **主进程**：创建 mempool/rings，克隆入队后通过 `rte_pktmbuf_free_bulk()` 批量释放原始数据包；未成功入队的 clone 单独 `rte_pktmbuf_free()` 释放
- **子进程**：从 ring 中 `rte_ring_dequeue_burst()` 批量出队，处理后通过 `rte_pktmbuf_free_bulk()` 批量释放 mbuf
- **零拷贝克隆**：`rte_pktmbuf_clone()` 共享数据缓冲区，只创建新的 mbuf 头

## 代码风格

- [`CMakeLists.txt`](CMakeLists.txt:4) 强制使用 C++11 标准，并启用 `-Wall -Wextra -O3`
- DPDK 命名规范：`rte_` 前缀函数，`snake_case` 变量名
- 全局 DPDK 对象：`ring_1`、`ring_2`、`mbuf_pool`（在 main.cpp 中定义）
- DPDK EAL 参数放在 `--` 之前，应用参数放在 `--` 之后
- 主/子进程均支持 SIGINT/SIGTERM 优雅退出，退出前释放 ring、mempool 并调用 `rte_eal_cleanup()`

## 项目特定注意事项

1. **必须用 `sudo` 运行** - DPDK 需要 root 权限访问 hugepages 和直接网卡访问
2. **主进程必须先启动** - 子进程在共享对象不存在时会失败（lookup 失败会直接退出并提示）
3. **Ring 标志很重要** - 多生产者/多消费者需要 `RTE_RING_F_MP_ENQ | RTE_RING_F_MC_DEQ`
4. **fast path 中不要有 stdout** - 绝不要在 `rx_loop()` 或子进程的出队循环中添加 printf，会严重降低性能
