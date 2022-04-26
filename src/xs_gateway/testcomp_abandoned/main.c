/*  programmer: luis miguel
    email: lmdelbahia@gmail.com  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "genqueue.h"
#include <pthread.h>

/*pthread_mutex_t mut;
pthread_cond_t cond;
int y=0;*/
Queue queue;
void *thfunc_deq(void *param)
{
    char buf[1000];
    fgets(buf, sizeof 1000, stdin);
    memset(buf, 0, sizeof buf);
    gq_dequeue_block(queue, buf, sizeof buf);
    printf("%s\n", buf);
    return NULL;
}

void *thfunc_enq(void *param)
{
    char buf[1000];
    fgets(buf, sizeof 1000, stdin);

    gq_enqueue_block(queue, "test", sizeof "test");
    
    return NULL;
}

pthread_t th;


int main(void)
{
    
    gq_create(&queue, 3);
    
    gq_enqueue_block(queue, "john", sizeof "john");
    gq_enqueue_block(queue, "albert", sizeof "albert");
    gq_enqueue_block(queue, "jim", sizeof "jim");
    
    pthread_create(&th, NULL, thfunc_enq, 0);
    
    //int y= gq_enqueue_block(queue, "jose", sizeof "jose");
    //printf("%d\n", y);
    
    char buf[100];
    memset(buf, 0, sizeof buf);
    gq_dequeue_block(queue, buf, sizeof buf);
    printf("%s\n", buf);
    
    memset(buf, 0, sizeof buf);
    gq_dequeue_block(queue, buf, sizeof buf);
    printf("%s\n", buf);
    
    memset(buf, 0, sizeof buf);
    gq_dequeue_block(queue, buf, sizeof buf);
    printf("%s\n", buf);
    
   /* memset(buf, 0, sizeof buf);
    gq_dequeue_block(queue, buf, sizeof buf);
    printf("%s\n", buf);*/
    
    gq_destroy(queue);
    
    
   /* pthread_mutex_init(&mut, NULL);
    pthread_cond_init(&cond, NULL);
    
    
    pthread_create(&th, NULL, thfun, NULL);
    sleep(2);
    //pthread_mutex_lock(&mut);
    
    //pthread_mutex_unlock(&mut);
    pthread_cond_signal(&cond);
    sleep(2);*/
    return 0;
}
