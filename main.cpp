#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <sys/queue.h>
#include <rte_common.h>
#include <rte_eal.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>

#define NUM_MBUFS        8191
#define RX_RING_SIZE     128
#define TX_RING_SIZE     512

static struct rte_mempool *mbuf_pool;

static int
init_mempool(void)
{
    const unsigned int socket_id = 0;
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS, 0, 0,
                                        RTE_MBUF_DEFAULT_BUF_SIZE, socket_id);
    if (mbuf_pool == NULL)
        return -1;
    return 0;
}

static int
init_port(uint16_t port_id)
{
    struct rte_eth_conf port_conf = {
       .rxmode = {
           .max_rx_pkt_len = RTE_ETHER_MAX_LEN,
           .offloads = DEV_RX_OFFLOAD_CHECKSUM | DEV_RX_OFFLOAD_JUMBO_FRAME,
        },
       .txmode = {
           .mq_mode = ETH_MQ_TX_NONE,
           .offloads = DEV_TX_OFFLOAD_MULTI_SEGS | DEV_TX_OFFLOAD_CHECKSUM,
        },
    };
    int retval;

    retval = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    if (retval != 0)
        return retval;

    retval = rte_eth_dev_adjust_nb_rx_tx_desc(port_id, RX_RING_SIZE, TX_RING_SIZE);
    if (retval != 0)
        return retval;

    /* 设置 RX/TX 队列（各 1 个，使用默认配置与 mempool） */
    retval = rte_eth_rx_queue_setup(port_id, 0, RX_RING_SIZE,
                                    rte_eth_dev_socket_id(port_id),
                                    NULL, mbuf_pool);
    if (retval != 0)
        return retval;

    retval = rte_eth_tx_queue_setup(port_id, 0, TX_RING_SIZE,
                                    rte_eth_dev_socket_id(port_id),
                                    NULL);
    if (retval != 0)
        return retval;

    retval = rte_eth_dev_start(port_id);
    if (retval != 0)
        return retval;

    return 0;
}

static int
receive_packets(uint16_t port_id)
{
    struct rte_mbuf *bufs[RX_RING_SIZE];
    const uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, RX_RING_SIZE);
    if (nb_rx == 0)
        return 0;
    for (uint16_t i = 0; i < nb_rx; i++) {
        /* 在此处理收到的数据包，当前直接释放 */
        rte_pktmbuf_free(bufs[i]);
    }
    return nb_rx;
}

int
main(int argc, char *argv[])
{
    uint16_t port_id = 0;
    int ret;

    /* 初始化 EAL，只调用一次并保存解析后的参数偏移 */
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");
    argc -= ret;
    argv += ret;

    if (init_mempool() < 0)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
    if (init_port(port_id) < 0)
        rte_exit(EXIT_FAILURE, "Cannot init port %u\n", port_id);

    while (1) {
        receive_packets(port_id);
        /* 发送逻辑：根据需要构造 mbuf 后调用 rte_eth_tx_burst() */
    }

    /* 不会执行到此处；如需优雅退出，请参考 dpdk_copy 的信号处理示例 */
    return 0;
}
