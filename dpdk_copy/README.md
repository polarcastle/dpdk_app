# DPDK 流量复制分发项目

基于 DPDK (Data Plane Development Kit) 的高性能网络数据包复制与多进程分发系统。

## 功能特性

- **流量复制**：将网卡接收的流量零拷贝克隆到多个处理队列
- **多进程处理**：支持多进程并行处理同一份流量
- **高性能**：利用 DPDK 的无锁环形队列和共享内存机制
- **零拷贝克隆**：使用 `rte_pktmbuf_clone()` 避免数据复制开销

## 项目结构

```
.
├── main.cpp         # 主进程 - 负责接收网卡流量并分发到两个 ring
├── subprocess.cpp   # 子进程 - 从 ring 读取并处理数据包
├── CMakeLists.txt   # CMake 构建配置文件
└── README.md        # 项目说明文档
```

## 编译要求

- DPDK 开发库 (>= 20.11)
- GCC/G++ 编译器
- meson & ninja (用于编译 DPDK)

### 方式一：使用 CMake (推荐)

```bash
# 创建构建目录
mkdir build && cd build

# 配置
cmake ..

# 编译
make -j$(nproc)

# 编译后的可执行文件在 build/ 目录下
```

### 方式二：直接编译

```bash
# 编译主进程
g++ -o main main.cpp $(pkg-config --cflags --libs libdpdk) -lpthread

# 编译子进程
g++ -o subprocess subprocess.cpp $(pkg-config --cflags --libs libdpdk) -lpthread
```

### 方式三：使用 Makefile

```makefile
CC = g++
CFLAGS = $(shell pkg-config --cflags libdpdk)
LDFLAGS = $(shell pkg-config --libs libdpdk) -lpthread

all: main subprocess

main: main.cpp
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

subprocess: subprocess.cpp
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f main subprocess
```

## 使用方法

### 1. 前置准备

确保已加载 DPDK 所需的内核模块并绑定网卡:

```bash
# 加载 UIO 或 VFIO 模块
sudo modprobe uio
sudo insmod /path/to/dpdk/build/kernel/linux/igb_uio/igb_uio.ko

# 绑定网卡到 DPDK 驱动
sudo /path/to/dpdk/usertools/dpdk-devbind.py --bind=igb_uio 0000:xx:00.0
```

### 2. 启动主进程 (Primary)

```bash
sudo ./main --proc-type=primary -l 0-3 -n 4
```

参数说明:
- `--proc-type=primary`: 指定为主进程
- `-l 0-3`: 使用 CPU 核心 0-3
- `-n 4`: 使用 4 个内存通道

### 3. 启动子进程 (Secondary)

```bash
# 终端 2 - 处理 ring_1 的流量
sudo ./subprocess --proc-type=secondary -l 4 -n 4 -- 1

# 终端 3 - 处理 ring_2 的流量
sudo ./subprocess --proc-type=secondary -l 5 -n 4 -- 2
```

参数说明:
- `--proc-type=secondary`: 指定为从进程
- `--` 之后的参数 `1` 或 `2`: 选择连接到 ring_1 或 ring_2（EAL 参数须放在 `--` 之前）

## 工作原理

### 架构图

```
┌─────────────────────────────────────────────────────────────┐
│                        主进程 (main)                          │
│  ┌──────────┐     ┌──────────────┐     ┌──────────────┐     │
│  │   网卡   │────▶│  RX 队列     │────▶│  mbuf_clone  │     │
│  └──────────┘     └──────────────┘     └──────┬───────┘     │
│                                               │             │
│                       ┌───────────────────────┘             │
│                       ▼                                     │
│              ┌─────────────────┬─────────────────┐          │
│              ▼                 ▼                 ▼          │
│         ┌─────────┐      ┌─────────┐      ┌─────────┐      │
│         │ ring_1  │      │ ring_2  │      │ 释放原包 │      │
│         └────┬────┘      └────┬────┘      └─────────┘      │
└──────────────┼────────────────┼─────────────────────────────┘
               │                │
               ▼                ▼
┌──────────────────────────┐  ┌──────────────────────────┐
│      子进程 1             │  │      子进程 2             │
│  ./subprocess 1          │  │  ./subprocess 2          │
│  从 ring_1 读取数据包      │  │  从 ring_2 读取数据包      │
│  处理并释放                │  │  处理并释放                │
└──────────────────────────┘  └──────────────────────────┘
```

### 核心机制

1. **共享内存池**: 主进程创建 `rte_mempool`，子进程通过名称查找并共享使用
2. **无锁环形队列**: 使用 `rte_ring` 实现多生产者/多消费者的安全队列
3. **数据包克隆**: `rte_pktmbuf_clone()` 创建 mbuf 的浅拷贝，共享实际数据缓冲区

### 数据流向

1. 主进程从网卡批量接收数据包 (`rte_eth_rx_burst`，每 burst 最多 32 个)
2. 对每个数据包调用 `rte_pktmbuf_clone()` 创建两个克隆（处理时预取下一个包）
3. 通过 `rte_ring_enqueue_burst()` 将克隆包批量放入 `ring_1` 和 `ring_2`
4. 通过 `rte_pktmbuf_free_bulk()` 批量释放原始数据包
5. 子进程用 `rte_ring_dequeue_burst()` 从各自的 ring 批量取出数据包，处理后批量释放

主/子进程均支持 SIGINT/SIGTERM 优雅退出，退出前完成资源清理（停端口、释放 ring/mempool、`rte_eal_cleanup()`）。

## 配置参数

### main.cpp

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `MEMPOOL_NAME` | 共享内存池名称 | "shared_mbuf_pool" |
| `RING_1_NAME` | 环形队列1名称 | "ring_1" |
| `RING_2_NAME` | 环形队列2名称 | "ring_2" |
| `mbuf_pool_size` | 内存池大小 | 8192 |
| `ring_size` | 队列大小 | 4096 |
| `rx_burst_size` | 每次接收包数 | 32 |

### 网卡配置

- **RX 队列**: 1 个队列，描述符数 128
- **TX 队列**: 1 个队列，描述符数 512
- **RSS 模式**: 启用多队列接收 (ETH_MQ_RX_RSS)

## 注意事项

1. **HugePages 配置**: 确保系统已配置足够的 HugePages
   ```bash
   echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
   ```

2. **权限**: 运行 DPDK 程序通常需要 root 权限

3. **进程启动顺序**: 必须先启动主进程，再启动子进程

4. **网卡绑定**: 网卡必须绑定到 DPDK 支持的驱动 (如 igb_uio, vfio-pci)

5. **内存管理**: 子进程结束后会自动释放从 ring 中 dequeue 的 mbuf

## 扩展建议

- 修改 `rx_loop()` 添加更多 ring 支持更多子进程
- 在子进程中实现自定义的数据包处理逻辑
- 添加统计信息输出（pps, bps 等）

## License

MIT License
