#include <rte_eal.h>
#include <rte_ring.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// 与主进程相同的共享数据结构名称
#define MEMPOOL_NAME "shared_mbuf_pool"
#define RING_1_NAME "ring_1"
#define RING_2_NAME "ring_2"

#define BURST_SIZE 32

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

int main(int argc, char *argv[]) {
    // 初始化EAL（注意：需添加 --proc-type=secondary 参数）
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "EAL init failed\n");
    argc -= ret;
    argv += ret;

    // EAL 参数解析后再读取应用参数：ring 编号 1 或 2
    if (argc < 2)
        rte_exit(EXIT_FAILURE, "Usage: ./subprocess [EAL options] -- <1|2>\n");

    const char *ring_id = argv[1];

    register_signals();

    // 查找主进程创建的共享内存池和ring
    rte_mempool *mbuf_pool = rte_mempool_lookup(MEMPOOL_NAME);
    if (mbuf_pool == nullptr)
        rte_exit(EXIT_FAILURE, "Cannot find mempool '%s' (is the primary process running?)\n",
                 MEMPOOL_NAME);

    rte_ring *ring = (strcmp(ring_id, "1") == 0) ?
                     rte_ring_lookup(RING_1_NAME) :
                     rte_ring_lookup(RING_2_NAME);
    if (ring == nullptr)
        rte_exit(EXIT_FAILURE, "Cannot find ring for id '%s'\n", ring_id);

    printf("Subprocess attached to ring %s, waiting for packets...\n", ring_id);

    // 批量出队并处理（fast path 中不做任何 printf）
    rte_mbuf *pkts[BURST_SIZE];
    while (!stop_flag) {
        unsigned int n = rte_ring_dequeue_burst(ring, (void **)pkts, BURST_SIZE, nullptr);
        if (n == 0)
            continue;

        // 在此添加数据包处理逻辑

        rte_pktmbuf_free_bulk(pkts, n);
    }

    rte_eal_cleanup();
    printf("Subprocess (ring %s) exited cleanly.\n", ring_id);
    return 0;
}
