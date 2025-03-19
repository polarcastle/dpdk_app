#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <cstring>
#include <thread>

// 定义共享内存中的数据结构名称
#define MEMPOOL_NAME "shared_mbuf_pool"
#define RING_1_NAME "ring_1"
#define RING_2_NAME "ring_2"

// 全局变量
rte_ring *ring_1, *ring_2;
rte_mempool *mbuf_pool;

// 初始化网卡
void init_port(uint16_t port_id) {
    struct rte_eth_conf port_conf = {};
    port_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
    port_conf.rxmode.max_rx_pkt_len = RTE_ETHER_MAX_LEN;

    rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    rte_eth_rx_queue_setup(port_id, 0, 128, rte_eth_dev_socket_id(port_id), nullptr);
    rte_eth_tx_queue_setup(port_id, 0, 512, rte_eth_dev_socket_id(port_id), nullptr);
    rte_eth_dev_start(port_id);
}

// 接收并复制流量到两个ring
void rx_loop(uint16_t port_id) {
    while (true) {
        rte_mbuf *rx_pkts[32];
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, rx_pkts, 32);

        for (uint16_t i = 0; i < nb_rx; i++) {
            // 克隆原始数据包（共享数据缓冲区）
            rte_mbuf *clone1 = rte_pktmbuf_clone(rx_pkts[i], mbuf_pool);
            rte_mbuf *clone2 = rte_pktmbuf_clone(rx_pkts[i], mbuf_pool);

            // 将克隆包放入两个不同的ring
            if (rte_ring_enqueue(ring_1, clone1) < 0)
                rte_pktmbuf_free(clone1);
            if (rte_ring_enqueue(ring_2, clone2) < 0)
                rte_pktmbuf_free(clone2);

            // 释放原始数据包
            rte_pktmbuf_free(rx_pkts[i]);
        }
    }
}

int main(int argc, char *argv[]) {
    // 初始化EAL（注意：需添加 --proc-type=primary 参数）
    rte_eal_init(argc, argv);

    // 创建共享内存池
    mbuf_pool = rte_pktmbuf_pool_create(
        MEMPOOL_NAME, 8192, 0, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id()
    );

    // 创建两个无锁环形队列（多生产者/多消费者模式）
    ring_1 = rte_ring_create(RING_1_NAME, 4096, rte_socket_id(), RTE_RING_F_MP_ENQ | RTE_RING_F_MC_DEQ);
    ring_2 = rte_ring_create(RING_2_NAME, 4096, rte_socket_id(), RTE_RING_F_MP_ENQ | RTE_RING_F_MC_DEQ);

    // 初始化网卡
    uint16_t port_id = 0;
    init_port(port_id);

    // 启动接收线程
    rx_loop(port_id);

    return 0;
}