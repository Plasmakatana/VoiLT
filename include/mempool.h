#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stddef.h>
#include <pthread.h>
#include "log.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

typedef struct mem_block {
    struct mem_block *next;
    void *data;
} mem_block_t;

typedef struct {
    size_t block_size;
    size_t blocks_per_pool;
    mem_block_t *free_list;
    pthread_mutex_t mutex;
    void **memory_blocks;
    size_t num_memory_blocks;
    size_t total_blocks;
    size_t used_blocks;
} mempool_t;

int mempool_init(mempool_t *pool, size_t block_size, size_t blocks_per_pool);
void *mempool_alloc(mempool_t *pool);
void mempool_free(mempool_t *pool, void *ptr);
void mempool_cleanup(mempool_t *pool);

#endif
