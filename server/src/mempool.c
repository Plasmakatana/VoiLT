#include "mempool.h"

static __thread mem_block_t *local_free_list = NULL;
static __thread int local_free_count = 0;
#define LOCAL_BATCH_SIZE 64

#define CACHE_LINE_SIZE 64

static void* allocate_memory(size_t size) {
    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t aligned_size = (size + page_size - 1) & ~(page_size - 1);

    if (aligned_size < 4096) {
        void* ptr;
        if (posix_memalign(&ptr, CACHE_LINE_SIZE, aligned_size) != 0) {
            return NULL;
        }
        return ptr;
    }

    void* ptr = mmap(NULL, aligned_size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED) {
        return NULL;
    }

    madvise(ptr, aligned_size, MADV_WILLNEED);

    return ptr;
}

static void free_memory(void* ptr, size_t size) {
    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t aligned_size = (size + page_size - 1) & ~(page_size - 1);

    if (aligned_size < 4096) {
        free(ptr);
    } else {
        munmap(ptr, aligned_size);
    }
}

static inline void prefetch_next_block(mem_block_t *block) {
    if (block && block->next) {
        __builtin_prefetch(block->next, 0, 3);
    }
}

int mempool_init(mempool_t *pool, size_t block_size, size_t blocks_per_pool) {
    if (!pool || block_size == 0 || blocks_per_pool == 0) {
        return -1;
    }

    block_size = (block_size + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);

    pool->block_size = block_size;
    pool->blocks_per_pool = blocks_per_pool;
    pool->free_list = NULL;
    pool->total_blocks = 0;
    pool->used_blocks = 0;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    if (pthread_mutex_init(&pool->mutex, &attr) != 0) {
        LOG_ERROR("Failed to initialize mutex");
        pthread_mutexattr_destroy(&attr);
        return -1;
    }
    pthread_mutexattr_destroy(&attr);

    size_t total_data_size = block_size * blocks_per_pool;
    char* block_memory = allocate_memory(total_data_size);

    if (!block_memory) {
        LOG_ERROR("Failed to allocate memory for blocks");
        pthread_mutex_destroy(&pool->mutex);
        return -1;
    }

    mem_block_t* block_headers = allocate_memory(sizeof(mem_block_t) * blocks_per_pool);
    if (!block_headers) {
        LOG_ERROR("Failed to allocate memory for block headers");
        free_memory(block_memory, total_data_size);
        pthread_mutex_destroy(&pool->mutex);
        return -1;
    }

    pool->memory_blocks = malloc(sizeof(void*) * 2);
    if (!pool->memory_blocks) {
        LOG_ERROR("Failed to allocate memory block array");
        free_memory(block_headers, sizeof(mem_block_t) * blocks_per_pool);
        free_memory(block_memory, total_data_size);
        pthread_mutex_destroy(&pool->mutex);
        return -1;
    }
    pool->memory_blocks[0] = block_memory;
    pool->memory_blocks[1] = block_headers;
    pool->num_memory_blocks = 2;

    for (size_t i = 0; i < blocks_per_pool; i++) {
        mem_block_t* block = &block_headers[i];
        block->data = (void*)(block_memory + (i * block_size));
        block->next = pool->free_list;
        pool->free_list = block;
    }

    pool->total_blocks = blocks_per_pool;

    LOG_DEBUG("Memory pool initialized with %zu blocks of %zu bytes (aligned to %d bytes)", 
             blocks_per_pool, block_size, CACHE_LINE_SIZE);

    return 0;
}

static void flush_local_cache(mempool_t *pool) {
    if (!local_free_list) return;

    pthread_mutex_lock(&pool->mutex);

    mem_block_t *last = local_free_list;
    int count = 1;
    while (last->next && count < local_free_count) {
        prefetch_next_block(last);
        last = last->next;
        count++;
    }

    last->next = pool->free_list;
    pool->free_list = local_free_list;

    pthread_mutex_unlock(&pool->mutex);

    local_free_list = NULL;
    local_free_count = 0;
}

static int refill_local_cache(mempool_t *pool) {
    pthread_mutex_lock(&pool->mutex);

    if (!pool->free_list) {
        size_t total_data_size = pool->block_size * pool->blocks_per_pool;
        char* block_memory = allocate_memory(total_data_size);

        if (!block_memory) {
            LOG_ERROR("Failed to grow memory pool");
            pthread_mutex_unlock(&pool->mutex);
            return -1;
        }

        mem_block_t* block_headers = allocate_memory(sizeof(mem_block_t) * pool->blocks_per_pool);
        if (!block_headers) {
            LOG_ERROR("Failed to allocate memory for block headers");
            free_memory(block_memory, total_data_size);
            pthread_mutex_unlock(&pool->mutex);
            return -1;
        }

        void** new_blocks = realloc(pool->memory_blocks, 
                                   (pool->num_memory_blocks + 2) * sizeof(void*));
        if (!new_blocks) {
            LOG_ERROR("Failed to resize memory blocks array");
            free_memory(block_headers, sizeof(mem_block_t) * pool->blocks_per_pool);
            free_memory(block_memory, total_data_size);
            pthread_mutex_unlock(&pool->mutex);
            return -1;
        }

        pool->memory_blocks = new_blocks;
        pool->memory_blocks[pool->num_memory_blocks] = block_memory;
        pool->memory_blocks[pool->num_memory_blocks + 1] = block_headers;
        pool->num_memory_blocks += 2;

        for (size_t i = 0; i < pool->blocks_per_pool; i++) {
            mem_block_t* block = &block_headers[i];
            block->data = (void*)(block_memory + (i * pool->block_size));
            block->next = pool->free_list;
            pool->free_list = block;

            if (i < pool->blocks_per_pool - 1) {
                __builtin_prefetch(&block_headers[i+1], 1, 3);
            }
        }

        pool->total_blocks += pool->blocks_per_pool;
        LOG_DEBUG("Memory pool expanded to %zu blocks", pool->total_blocks);
    }

    local_free_list = pool->free_list;
    mem_block_t *last = local_free_list;
    int count = 1;

    while (count < LOCAL_BATCH_SIZE && last->next) {
        prefetch_next_block(last);
        last = last->next;
        count++;
    }

    pool->free_list = last->next;
    last->next = NULL;
    local_free_count = count;

    pthread_mutex_unlock(&pool->mutex);

    return 0;
}

void* mempool_alloc(mempool_t *pool) {
    if (!local_free_list) {
        if (refill_local_cache(pool) != 0) {
            return NULL;
        }

        if (!local_free_list) {
            return NULL;
        }
    }

    mem_block_t *block = local_free_list;
    local_free_list = block->next;
    local_free_count--;

    prefetch_next_block(local_free_list);

    __atomic_add_fetch(&pool->used_blocks, 1, __ATOMIC_SEQ_CST);

    return block->data;
}

void mempool_free(mempool_t *pool, void *ptr) {
    if (!pool || !ptr) {
        return;
    }

    int found = 0;
    for (size_t i = 0; i < pool->num_memory_blocks; i += 2) {
        char *block_memory = (char *)pool->memory_blocks[i];
        size_t total_data_size = pool->block_size * pool->blocks_per_pool;

        uintptr_t ptr_addr = (uintptr_t)ptr;
        uintptr_t block_start = (uintptr_t)block_memory;
        uintptr_t block_end = block_start + total_data_size;

        if (ptr_addr >= block_start && ptr_addr < block_end) {
            size_t block_index = ((char *)ptr - block_memory) / pool->block_size;

            mem_block_t *block_headers = (mem_block_t *)pool->memory_blocks[i + 1];
            mem_block_t *block = &block_headers[block_index];

            block->next = local_free_list;
            local_free_list = block;
            local_free_count++;

            __atomic_sub_fetch(&pool->used_blocks, 1, __ATOMIC_SEQ_CST);

            found = 1;
            break;
        }
    }

    if (!found) {
        LOG_ERROR("Attempted to free invalid pointer: %p", ptr);
        return;
    }

    if (local_free_count >= LOCAL_BATCH_SIZE * 2) {
        flush_local_cache(pool);
    }
}

void mempool_cleanup(mempool_t *pool) {
    if (!pool) {
        return;
    }

    flush_local_cache(pool);

    pthread_mutex_lock(&pool->mutex);

    for (size_t i = 0; i < pool->num_memory_blocks; i += 2) {
        size_t total_size = pool->block_size * pool->blocks_per_pool;
        free_memory(pool->memory_blocks[i], total_size);

        if (i + 1 < pool->num_memory_blocks) {
            free(pool->memory_blocks[i + 1]);
        }
    }

    free(pool->memory_blocks);
    pool->memory_blocks = NULL;
    pool->num_memory_blocks = 0;
    pool->free_list = NULL;
    pool->total_blocks = 0;
    pool->used_blocks = 0;

    pthread_mutex_unlock(&pool->mutex);
    pthread_mutex_destroy(&pool->mutex);

    LOG_DEBUG("Memory pool cleaned up");
}
