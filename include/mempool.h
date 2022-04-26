/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stdlib.h>

/* Memory pool structure */
struct mempool {
    void *data;
    size_t size;
    size_t blksz;
    int count;
};

/* Create memory pool with fixed block size. */
void mpool_create(struct mempool *pool, size_t blksz);
/* Destroy memory pool. */
void mpool_destroy(struct mempool *pool);
/* Add new block of memory. There is not remove function, so to free memory
    mpool_destroy must be called. Every time this function is called the
    address of pool->data can change due realloc function. Every pointer that
    points to pool->data or its offset, could be obsolete after the call. */
void *mpool_addblk(struct mempool *pool);

#endif
