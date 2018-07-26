#include <rte_eal.h>
#include <rte_common.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_errno.h>
#include <rte_cycles.h>
#include <rte_jhash.h>
#include <rte_hash_crc.h>
#include "./xxhash.h"

#define MBUF_PER_POOL 2 << 16
#define MBUF_POOL_CACHE_SIZE 250
#define JUMBO_FRAME_MAX_SIZE  0x2600
#undef RTE_MBUF_DEFAULT_BUF_SIZE
#define RTE_MBUF_DEFAULT_BUF_SIZE (JUMBO_FRAME_MAX_SIZE + RTE_PKTMBUF_HEADROOM)

struct rte_mempool *mbuf_pool;
struct rte_mbuf *bufs[MBUF_PER_POOL];


static int
jhash_perf(void) {
  uint64_t begin = rte_get_tsc_cycles();
  uint16_t a[0xff] = {0};

  for(uint64_t i = 0; i < MBUF_PER_POOL; i++) {
    a[rte_jhash(rte_pktmbuf_mtod(bufs[i], uint8_t *), bufs[i]->pkt_len, 0) & 0xff] += 1;
  }

  printf("jhash: %"PRIu64"\n", rte_get_tsc_cycles() - begin);
  for(uint16_t j = 0; j < 0xff; j++) {
    printf("%u:%u ", j, a[j]);
  }
  printf("\n");

  return 0;
}


static int
crc_perf(void) {
  uint64_t begin = rte_get_tsc_cycles();
  uint16_t a[0xff] = {0};

  for(uint64_t i = 0; i < MBUF_PER_POOL; i++) {
    a[rte_hash_crc(rte_pktmbuf_mtod(bufs[i], uint8_t *), bufs[i]->pkt_len, 0) & 0xff] += 1;
  }

  printf("crc: %"PRIu64"\n", rte_get_tsc_cycles() - begin);
  for(uint16_t j = 0; j < 0xff; j++) {
    printf("%u:%u ", j, a[j]);
  }
  printf("\n");

  return 0;

}


static int
xxh_perf(void) {
  uint64_t begin = rte_get_tsc_cycles();
  uint16_t a[0xff] = {0};

  for(uint64_t i = 0; i < MBUF_PER_POOL; i++) {
    a[XXH64(rte_pktmbuf_mtod(bufs[i], uint64_t *), bufs[i]->pkt_len/8, 0) & 0xff] += 1;
  }

  printf("xxh64: %"PRIu64"\n", rte_get_tsc_cycles() - begin);
  for(uint16_t j = 0; j < 0xff; j++) {
    printf("%u:%u ", j, a[j]);
  }
  printf("\n");

  return 0;


}



int
main(int argc, char **argv) {
  int ret;
  ret = rte_eal_init(argc, argv);
  if (ret < 0) {
    return -1;
  }

  argc -= ret;
  argv += ret;


  mbuf_pool = rte_pktmbuf_pool_create(
    "mbuf_pool",
    MBUF_PER_POOL,
    MBUF_POOL_CACHE_SIZE,
    0,
    RTE_MBUF_DEFAULT_BUF_SIZE,
    rte_socket_id()
  );
  if(mbuf_pool == NULL) {
    rte_exit(EXIT_FAILURE, "%s\n", rte_strerror(rte_errno));
  }

  for(uint64_t i = 0; i < MBUF_PER_POOL; i++) {
    if((bufs[i] = rte_pktmbuf_alloc(mbuf_pool)) == NULL) {
      rte_exit(EXIT_FAILURE, "%s\n", rte_strerror(rte_errno));
    }
    rte_pktmbuf_append(bufs[i], 9000);

    uint64_t *x;
    x = rte_pktmbuf_mtod(bufs[i], uint64_t *);
    *x = rte_get_tsc_cycles();
  }

  jhash_perf();
  jhash_perf();
  jhash_perf();
  jhash_perf();
  jhash_perf();

  crc_perf();
  crc_perf();
  crc_perf();
  crc_perf();
  crc_perf();

  xxh_perf();
  xxh_perf();
  xxh_perf();
  xxh_perf();
  xxh_perf();


  return 0;
}
