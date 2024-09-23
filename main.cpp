#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <sys/queue.h>
#include <rte_common.h>
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
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS, 0, 0, RTE_MBUF_DEFAULT_BUF_SIZE, socket_id);
    if (mbuf_pool == NULL)
        return -1;
    return 0;
}

static int
init_port(uint16_t port_id)
{
    struct rte_eth_conf port_conf = {
       .rxmode = {
           .max_rx_pkt_len = ETHER_MAX_LEN,
           .split_hdr_size = 0,
           .offloads = DEV_RX_OFFLOAD_CHECKSUM | DEV_RX_OFFLOAD_JUMBO_FRAME,
        },
       .txmode = {
           .mq_mode = ETH_MQ_TX_NONE,
           .offloads = DEV_TX_OFFLOAD_MULTI_SEGS | DEV_TX_OFFLOAD_CHECKSUM,
        },
    };
    int retval;
    retval = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    if (retval!= 0)
        return retval;
    retval = rte_eth_dev_adjust_nb_rx_tx_desc(port_id, RX_RING_SIZE, TX_RING_SIZE);
    if (retval!= 0)
        return retval;
    retval = rte_eth_dev_start(port_id);
    if (retval!= 0)
        return retval;
    return 0;
}

static int
receive_packets(uint16_t port_id)
{
    struct rte_mbuf *bufs[RX_RING_SIZE];
    const unsigned int nb_rx = rte_eth_rx_burst(port_id, 0, bufs, RX_RING_SIZE);
    if (nb_rx == 0)
        return 0;
    for (unsigned int i = 0; i < nb_rx; i++) {
        // Process received packet here
        rte_pktmbuf_free(bufs[i]);
    }
    return nb_rx;
}

static int
send_packets(uint16_t port_id, struct rte_mbuf *bufs[], unsigned int nb_pkts)
{
    const unsigned int nb_tx = rte_eth_tx_burst(port_id, 0, bufs, nb_pkts);
    if (nb_tx < nb_pkts) {
        // Retry sending remaining packets
        for (unsigned int i = nb_tx; i < nb_pkts; i++) {
            rte_pktmbuf_free(bufs[i]);
        }
    }
    return nb_tx;
}

int
main(int argc, char *argv[])
{
    uint16_t port_id = 0;
    if (rte_eal_init(argc, argv) < 0)
        return -1;
    argc -= rte_eal_init(argc, argv);
    argv += rte_eal_init(argc, argv);
    if (init_mempool() < 0)
        return -1;
    if (init_port(port_id) < 0)
        return -1;
    while (1) {
        receive_packets(port_id);
        // Process packets and prepare for sending
        struct rte_mbuf *bufs[TX_RING_SIZE];
        // Populate bufs with packets to send
        send_packets(port_id, bufs, sizeof(bufs) / sizeof(bufs[0]));
    }
    return 0;
}