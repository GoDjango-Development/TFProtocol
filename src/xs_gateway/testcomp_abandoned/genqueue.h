/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

/* GENQUEUE Implements a circular single-linked list to create a queue. */

#ifndef GENQUEUE_H
#define GENQUEUE_H

/* Returned on anything involving a null parameter. */
#define GQ_QNULL -1
/* Returned when trying to enqueue and the queue is full. */
#define GQ_QFULL -2
/* Returned when the data can't be enqueue or dequeue due lack of memory. */
#define GQ_QMEMORY -3
/* Returned when trying to dequeue and the queue is empty. */
#define GQ_QEMPTY -4
/* Error setting synchronization primitive type. */
#define GQ_QTYPEFAIL -5;
/* Returned on successful operation. */
#define GQ_OK 0

typedef struct queue *Queue;

/* GQ_BLOCK_SHARED: multi-process aware;
    GQ_BLOCK_PRIVATE: multi-thread aware; */
typedef enum gq_block { GQ_BLOCK_SHARED, GQ_BLOCK_PRIVATE } gq_block;

/* Create a queue. */
void gq_create(Queue *handle, int sz);
/* Destroy a queue. */
void gq_destroy(Queue handle);
/* Enqueue some data. */
int gq_enqueue(Queue handle, void *data, int sz);
/* Dequeue some data. */
int gq_dequeue(Queue handle, void *data, int sz);
/* Enqueue blocking version: if queue is empty, it will wait until there is
    available data. */
int gq_enqueue_block(Queue handle, void *data, int sz);
/* Dequeue blocking version: if queue is full, it will wait until there is
    available space. */
int gq_dequeue_block(Queue handle, void *data, int sz);
/* Set the type of the synchronization primitive. */
int gq_setblocktype(Queue handle, gq_block type);

#endif
