/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <mempool.h>
#include <malloc.h>
#include <string.h>

void mpool_create(struct mempool *pool, size_t blksz)
{
    memset(pool, 0, sizeof(struct mempool));
    pool->data = NULL;
    pool->blksz = blksz;
    pool->size = 0;
    pool->count = 0;
}

void mpool_destroy(struct mempool *pool)
{
    if (pool->data) {
        free(pool->data);
        mpool_create(pool, 0);
    }
}

void *mpool_addblk(struct mempool *pool)
{
    void *data = NULL;
    data = realloc(pool->data, pool->size + pool->blksz);
    if (!data) {
        free(pool->data);
        return NULL;
    }
    pool->data = data;
    pool->count++;
    pool->size += pool->blksz;
    memset(pool->data + (pool->count - 1) * pool->blksz, 0, pool->blksz);
    return pool->data;
}
