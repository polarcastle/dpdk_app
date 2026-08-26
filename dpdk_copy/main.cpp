#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_prefetch.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// 定义共享内存中的数据结构名称
#define MEMPOOL_NAME "shared_mbuf_pool"
#define RING_1_NAME "ring_1"
#define RING_2_NAME "ring_2"

#define MBUF_POOL_SIZE 8192
#define RING_SIZE      4096
#define BURST_SIZE     32

// 全局变量
static rte_ring *ring_1, *ring_2;
static rte_mempool *mbuf_pool;

// 优雅退出标志（信号处理函数中设置）
static volatile sig_atomic_t stop_flag = 0;

static void handle_signal(int signo) {
    (void)signo;
    stop_flag = 1;
}

static void register_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

// 初始化网卡
static void init_port(uint16_t port_id) {
    struct rte_eth_conf port_conf = {};
    port_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
    port_conf.rxmode.max_rx_pkt_len = RTE_ETHER_MAX_LEN;

    int ret = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    if (ret != 0)
        rte_exit(EXIT_FAILURE, "rte_eth_dev_configure failed: %s\n", strerror(-ret));

    ret = rte_eth_rx_queue_setup(port_id, 0, 128, rte_eth_dev_socket_id(port_id),
                                 nullptr, mbuf_pool);
    if (ret != 0)
        rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup failed: %s\n", strerror(-ret));

    ret = rte_eth_tx_queue_setup(port_id, 0, 512, rte_eth_dev_socket_id(port_id),
                                 nullptr);
    if (ret != 0)
        rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup failed: %s\n", strerror(-ret));

    ret = rte_eth_dev_start(port_id);
    if (ret != 0)
        rte_exit(EXIT_FAILURE, "rte_eth_dev_start failed: %s\n", strerror(-ret));
}

// 接收并复制流量到两个ring（批量入队 + 预取）
static void rx_loop(uint16_t port_id) {
    rte_mbuf *rx_pkts[BURST_SIZE];
    rte_mbuf *to_ring_1[BURST_SIZE];
    rte_mbuf *to_ring_2[BURST_SIZE];

    while (!stop_flag) {
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, rx_pkts, BURST_SIZE);
        if (nb_rx == 0)
            continue;

        uint16_t n1 = 0, n2 = 0;
        for (uint16_t i = 0; i < nb_rx; i++) {
            // 预取下一个包的数据缓存行
            if (i + 1 < nb_rx)
                rte_prefetch0(rte_pktmbuf_mtod(rx_pkts[i + 1], void *));

            // 克隆原始数据包（共享数据缓冲区）
            rte_mbuf *clone1 = rte_pktmbuf_clone(rx_pkts[i], mbuf_pool);
            rte_mbuf *clone2 = rte_pktmbuf_clone(rx_pkts[i], mbuf_pool);

            if (clone1 != nullptr)
                to_ring_1[n1++] = clone1;
            if (clone2 != nullptr)
                to_ring_2[n2++] = clone2;
        }

        // 批量入队，未成功入队的 clone 单独释放
        unsigned int n_enq1 = rte_ring_enqueue_burst(ring_1, (void **)to_ring_1, n1, nullptr);
        unsigned int n_enq2 = rte_ring_enqueue_burst(ring_2, (void **)to_ring_2, n2, nullptr);
        for (unsigned int i = n_enq1; i < n1; i++)
            rte_pktmbuf_free(to_ring_1[i]);
        for (unsigned int i = n_enq2; i < n2; i++)
            rte_pktmbuf_free(to_ring_2[i]);

        // 批量释放原始数据包
        rte_pktmbuf_free_bulk(rx_pkts, nb_rx);
    }
}

int main(int argc, char *argv[]) {
    // 初始化EAL（注意：需添加 --proc-type=primary 参数）
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");
    argc -= ret;
    argv += ret;

    register_signals();

    // 创建共享内存池
    mbuf_pool = rte_pktmbuf_pool_create(
        MEMPOOL_NAME, MBUF_POOL_SIZE, 0, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id()
    );
    if (mbuf_pool == nullptr)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    // 创建两个无锁环形队列（多生产者/多消费者模式）
    ring_1 = rte_ring_create(RING_1_NAME, RING_SIZE, rte_socket_id(),
                             RTE_RING_F_MP_ENQ | RTE_RING_F_MC_DEQ);
    ring_2 = rte_ring_create(RING_2_NAME, RING_SIZE, rte_socket_id(),
                             RTE_RING_F_MP_ENQ | RTE_RING_F_MC_DEQ);
    if (ring_1 == nullptr || ring_2 == nullptr)
        rte_exit(EXIT_FAILURE, "Cannot create rings\n");

    // 初始化网卡
    uint16_t port_id = 0;
    init_port(port_id);

    // 接收循环（收到 SIGINT/SIGTERM 后退出）
    rx_loop(port_id);

    // 资源清理
    rte_eth_dev_stop(port_id);
    rte_eth_dev_close(port_id);
    rte_ring_free(ring_1);
    rte_ring_free(ring_2);
    rte_mempool_free(mbuf_pool);
    rte_eal_cleanup();

    printf("Primary process exited cleanly.\n");
    return 0;
}
