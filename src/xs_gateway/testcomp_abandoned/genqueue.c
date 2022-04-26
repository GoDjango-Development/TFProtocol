/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <malloc.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "genqueue.h"

struct node {
    int sz;
    int len;
    void *data;
    volatile sig_atomic_t used;
    struct node *nx;
};

struct queue {
    struct node *deq;
    struct node *enq;
    pthread_mutex_t enqmut;
    pthread_mutex_t deqmut;
    pthread_cond_t enqcond;
    pthread_cond_t deqcond;
};

void gq_create(Queue *handle, int sz)
{
    int c = 0;
    *handle = malloc(sizeof(struct queue));
    if (!*handle)
        return;
    struct node *node;
    struct node **pnode = &node;
    for (; c < sz; c++) {
        *pnode = malloc(sizeof(struct node));
        if (!*pnode) {
            (*handle)->deq = node;
            gq_destroy(*handle);
            *handle = NULL;
            return;
        }
        (*pnode)->sz = c;
        (*pnode)->len = 0;
        (*pnode)->used = 0;
        (*pnode)->data = NULL;
        (*pnode)->nx = NULL;
        pnode = &(*pnode)->nx;
    }
    *pnode = node;
    (*handle)->deq = node;
    (*handle)->enq = node;
    int rc = pthread_mutex_init(&(*handle)->enqmut, NULL);
    rc += pthread_mutex_init(&(*handle)->deqmut, NULL);
    rc += pthread_cond_init(&(*handle)->enqcond, NULL);
    rc += pthread_cond_init(&(*handle)->deqcond, NULL);
    if (rc) {
        gq_destroy(*handle);
        *handle = NULL;
    }
}

void gq_destroy(Queue handle)
{
    if (handle) {
        struct node *node = handle->deq;
        if (node) {
            do {
                if (node->data)
                    free(node->data);
                void *pnx = node->nx;
                free(node);
                node = pnx;
            } while (node && node != handle->deq);
        }
        pthread_mutex_destroy(&handle->enqmut);
        pthread_mutex_destroy(&handle->deqmut);
        pthread_cond_destroy(&handle->enqcond);
        pthread_cond_destroy(&handle->deqcond);
        free(handle);
    }
}

int gq_enqueue(Queue handle, void *data, int sz)
{
    if (!handle || !data)
        return GQ_QNULL;
    if (sz == 0)
        return GQ_OK;
    if (handle->enq->used)
        return GQ_QFULL;
    if (handle->enq->sz < sz) {
        free(handle->enq->data);
        handle->enq->data = malloc(sz);
        if (!handle->enq->data)
            return GQ_QMEMORY;
        handle->enq->sz = sz;
    }
    memcpy(handle->enq->data, data, sz);
    handle->enq->len = sz;
    handle->enq->used = 1;
    handle->enq = handle->enq->nx;
    return GQ_OK;
}

int gq_dequeue(Queue handle, void *data, int sz)
{
    if (!handle || !data)
        return GQ_QNULL;
    if (sz == 0)
        return GQ_OK;
    if (!handle->deq->used)
        return GQ_QEMPTY;
    if (handle->deq->len > sz)
        return GQ_QMEMORY;
    memcpy(data, handle->deq->data, handle->deq->len);
    handle->deq->len = 0;
    handle->deq->used = 0;
    handle->deq = handle->deq->nx;
    return GQ_OK;
}

int gq_enqueue_block(Queue handle, void *data, int sz)
{
    if (!handle || !data)
        return GQ_QNULL;
    if (sz == 0)
        return GQ_OK;
    pthread_mutex_lock(&handle->enqmut);
    if (handle->enq->used)
        pthread_cond_wait(&handle->enqcond, &handle->enqmut);
    pthread_mutex_unlock(&handle->enqmut);
    if (handle->enq->sz < sz) {
        free(handle->enq->data);
        handle->enq->data = malloc(sz);
        if (!handle->enq->data)
            return GQ_QMEMORY;
        handle->enq->sz = sz;
    }
    memcpy(handle->enq->data, data, sz);
    handle->enq->len = sz;
    pthread_mutex_lock(&handle->deqmut);
    handle->enq->used = 1;
    pthread_cond_signal(&handle->deqcond);
    pthread_mutex_unlock(&handle->deqmut);
    handle->enq = handle->enq->nx;
    return GQ_OK;
}

int gq_dequeue_block(Queue handle, void *data, int sz)
{
    if (!handle || !data)
        return GQ_QNULL;
    if (sz == 0)
        return GQ_OK;
    pthread_mutex_lock(&handle->deqmut);
    if (!handle->deq->used)
        pthread_cond_wait(&handle->deqcond, &handle->deqmut);
    pthread_mutex_unlock(&handle->deqmut);
    if (handle->deq->len > sz)
        return GQ_QMEMORY;
    memcpy(data, handle->deq->data, handle->deq->len);
    handle->deq->len = 0;
    pthread_mutex_lock(&handle->enqmut);
    handle->deq->used = 0;
    pthread_cond_signal(&handle->enqcond);
    pthread_mutex_unlock(&handle->enqmut);
    handle->deq = handle->deq->nx;
    return GQ_OK;
}

int gq_setblocktype(Queue handle, gq_block type)
{
    switch (type) {
        case GQ_BLOCK_SHARED:
            break;
        case GQ_BLOCK_PRIVATE:
            break;
    }
    return 0;
}
