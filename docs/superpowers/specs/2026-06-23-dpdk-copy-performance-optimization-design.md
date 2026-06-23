# dpdk_copy 高性能数据面优化设计文档

## 背景与目标

`dpdk_copy` 是一个基于 DPDK 的多进程流量复制分发示例：主进程从网卡接收数据包，对每个包做两份 `rte_pktmbuf_clone()`，分别入队到 `ring_1` 和 `ring_2`；两个 secondary 子进程从各自的 ring 出队并处理。

当前实现存在明显的数据面低效点：
- 主进程对每个 clone 单独调用 `rte_ring_enqueue`；
- 子进程每次只出队一个 mbuf，且 fast path 内调用 `printf`；
- `rte_eal_init` 返回值未处理，导致子进程 `argv[1]` 解析可能错位；
- 缺少错误处理与优雅退出机制。

本次优化的核心目标是：**在保留现有「主进程 + 两个 secondary 子进程」架构的前提下，最大化每秒包数（PPS）**，同时提升代码健壮性。

## 设计范围

### 包含

- `dpdk_copy/main.cpp` 的 RX 循环优化、批量入队、预取、错误处理、优雅退出；
- `dpdk_copy/subprocess.cpp` 的批量出队、参数解析修正、错误处理；
- `dpdk_copy/CMakeLists.txt` 的编译选项加固（如 `-Wall -Wextra`）。

### 不包含

- 引入多 RX 队列或 RSS；
- 改为单进程多线程；
- 支持动态 N 个 ring；
- 新增自动化测试框架；
- 新增统计/监控 Web 界面。

## 架构

保持现有进程模型：

```
┌─────────────────────────────────────────┐
│           主进程 (main)                  │
│  NIC RX ──► clone x2 ──► ring_1/ring_2   │
└──────────────────┬──────────────────────┘
                   │
      ┌────────────┴────────────┐
      ▼                         ▼
┌─────────────┐           ┌─────────────┐
│ subprocess 1 │           │ subprocess 2 │
│ dequeue_burst│           │ dequeue_burst│
│ free_bulk    │           │ free_bulk    │
└─────────────┘           └─────────────┘
```

## 关键改动

### 1. 主进程 `main.cpp`

#### 1.1 EAL 参数解析

```cpp
int ret = rte_eal_init(argc, argv);
if (ret < 0) {
    rte_exit(EXIT_FAILURE, "EAL init failed\n");
}
argc -= ret;
argv += ret;
```

#### 1.2 批量入队

RX burst 内维护两个临时数组 `to_ring_1[]` 和 `to_ring_2[]`，批量大小与 RX burst 大小一致，固定为 32：

```cpp
const uint16_t burst_size = 32;
rte_mbuf *to_ring_1[burst_size];
rte_mbuf *to_ring_2[burst_size];
uint16_t n1 = 0, n2 = 0;

for (uint16_t i = 0; i < nb_rx; i++) {
    if (i + 1 < nb_rx) {
        rte_prefetch0(rte_pktmbuf_mtod(rx_pkts[i + 1], void *));
    }

    rte_mbuf *clone1 = rte_pktmbuf_clone(rx_pkts[i], mbuf_pool);
    rte_mbuf *clone2 = rte_pktmbuf_clone(rx_pkts[i], mbuf_pool);

    if (clone1) to_ring_1[n1++] = clone1;
    if (clone2) to_ring_2[n2++] = clone2;
}

unsigned int n_enq1 = rte_ring_enqueue_burst(ring_1, (void **)to_ring_1, n1, nullptr);
unsigned int n_enq2 = rte_ring_enqueue_burst(ring_2, (void **)to_ring_2, n2, nullptr);

for (unsigned int i = n_enq1; i < n1; i++) rte_pktmbuf_free(to_ring_1[i]);
for (unsigned int i = n_enq2; i < n2; i++) rte_pktmbuf_free(to_ring_2[i]);

rte_pktmbuf_free_bulk(rx_pkts, nb_rx);
```

未成功入队的 mbuf 需要单独释放。

#### 1.3 预取

在处理第 `i` 个包时预取第 `i+1` 个包的数据缓存行，减少缓存未命中。

#### 1.4 优雅退出

注册 `SIGINT`/`SIGTERM` 处理函数，设置全局标志位；主循环检测到标志位后跳出，释放资源并调用 `rte_eal_cleanup()`。

### 2. 子进程 `subprocess.cpp`

#### 2.1 EAL 参数解析

与主进程一致，正确处理 `rte_eal_init` 返回值后再读取应用参数：

```cpp
int ret = rte_eal_init(argc, argv);
if (ret < 0) {
    rte_exit(EXIT_FAILURE, "EAL init failed\n");
}
argc -= ret;
argv += ret;

if (argc < 2) {
    rte_exit(EXIT_FAILURE, "Usage: ./subprocess <1|2>\n");
}
```

#### 2.2 批量出队

```cpp
const uint16_t burst_size = 32;
rte_mbuf *pkts[burst_size];
while (!stop) {
    unsigned int n = rte_ring_dequeue_burst(ring, (void **)pkts, burst_size, nullptr);
    if (n == 0) continue;

    // 处理逻辑（当前为空，保持示例最小化）

    rte_pktmbuf_free_bulk(pkts, n);
}
```

#### 2.3 移除 fast path printf

删除 RX 循环内的 `printf`，避免系统调用拖慢数据面。本次优化不新增持续统计输出，保持示例最小化。

### 3. 编译选项

在 `dpdk_copy/CMakeLists.txt` 中为两个目标追加：

```cmake
target_compile_options(${target} PRIVATE -Wall -Wextra -O3)
```

## 数据流

```
NIC RX burst (<=32)
        │
        ▼
┌──────────────────────┐
│ rte_eth_rx_burst()   │
└──────────┬───────────┘
           │
           ▼
┌─────────────────────────────────────┐
│ 对每个 mbuf 做 rte_pktmbuf_clone x2  │
│ 预取下一个包的数据                    │
└──────────┬──────────────┬───────────┘
           │              │
           ▼              ▼
   to_ring_1[]      to_ring_2[]
           │              │
           ▼              ▼
   rte_ring_enqueue_burst
           │              │
           ▼              ▼
      ring_1          ring_2
           │              │
           ▼              ▼
   rte_ring_dequeue_burst
           │              │
           ▼              ▼
   rte_pktmbuf_free_bulk
```

## 错误处理

| 场景 | 处理 |
|------|------|
| `rte_eal_init` 失败 | 打印错误并退出 |
| `rte_pktmbuf_pool_create` 失败 | 打印错误并退出 |
| `rte_ring_create` 失败 | 打印错误并退出 |
| `rte_eth_dev_configure`/`rx_queue_setup`/`tx_queue_setup`/`dev_start` 失败 | 打印错误并退出 |
| `rte_mempool_lookup`/`rte_ring_lookup` 失败 | 打印错误并退出 |
| `rte_ring_enqueue_burst` 未全部成功 | 释放未入队的 clone |
| clone 失败 | 仅释放已创建 clone，记录错误计数 |

## 资源清理

主进程退出时：
1. 停止网卡端口 `rte_eth_dev_stop(port_id)`；
2. 释放两个 ring `rte_ring_free(ring_1/ring_2)`；
3. 释放 mempool `rte_mempool_free(mbuf_pool)`；
4. 调用 `rte_eal_cleanup()`。

子进程退出时：
1. 调用 `rte_eal_cleanup()`。

## 测试与验证

### 编译验证

```bash
cd /home/lxy/dpdk_app/dpdk_copy/build
cmake ..
make -j$(nproc)
```

以及根项目统一构建：

```bash
cmake -S /home/lxy/dpdk_app -B /home/lxy/dpdk_app/build -G Ninja
cmake --build /home/lxy/dpdk_app/build -- -j$(nproc)
```

### 运行时验证（需 DPDK 环境）

1. 配置 HugePages；
2. 绑定网卡到 DPDK 驱动；
3. 启动主进程：
   ```bash
   sudo ./main --proc-type=primary -- -l 0 -n 4
   ```
4. 启动两个子进程：
   ```bash
   sudo ./subprocess --proc-type=secondary -- -l 1 -n 4 1
   sudo ./subprocess --proc-type=secondary -- -l 2 -n 4 2
   ```
5. 观察：
   - 子进程不再大量打印；
   - `SIGINT` 后主进程能正常退出；
   - 运行一段时间后 mempool 未使用计数回到初始值（无泄漏）。

## 风险与回退

| 风险 | 缓解措施 |
|------|----------|
| DPDK API 版本差异导致某些函数不可用 | 使用 DPDK 20.11+ 通用 API，如 `rte_ring_enqueue_burst`；编译失败时回退到单包 API |
| 批量释放时 mbuf 引用计数异常 | 保持 clone 与原始包一一对应，释放路径不变 |
| 信号处理与 DPDK 内部信号冲突 | 使用 `sigaction` 注册，避免覆盖 DPDK 关键处理 |

## 成功标准

- 代码可编译通过，无新增编译告警；
- 子进程 fast path 无 `printf`；
- 主进程/子进程均使用批量 ring 操作；
- `rte_eal_init` 参数解析正确；
- 存在优雅退出路径与资源清理。
