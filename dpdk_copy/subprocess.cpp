#include <rte_eal.h>
#include <rte_ring.h>
#include <rte_mbuf.h>
#include <cstdio>

// 与主进程相同的共享数据结构名称
#define MEMPOOL_NAME "shared_mbuf_pool"
#define RING_1_NAME "ring_1"
#define RING_2_NAME "ring_2"

int main(int argc, char *argv[]) {
    // 初始化EAL（注意：需添加 --proc-type=secondary 参数）
    rte_eal_init(argc, argv);

    // 查找主进程创建的共享内存池和ring
    rte_mempool *mbuf_pool = rte_mempool_lookup(MEMPOOL_NAME);
    rte_ring *ring = (strcmp(argv[1], "1") == 0) ? 
                     rte_ring_lookup(RING_1_NAME) : 
                     rte_ring_lookup(RING_2_NAME);

    // 从ring中读取数据包
    while (true) {
        rte_mbuf *mbuf;
        if (rte_ring_dequeue(ring, (void **)&mbuf) == 0) {
            // 处理数据包（示例：打印长度）
            printf("Process %s received packet, len=%u\n", argv[1], mbuf->pkt_len);
            rte_pktmbuf_free(mbuf);
        }
    }

    return 0;
}