#include"../include/apue.h"
#include<pthread.h>

typedef struct _node{
    struct _node *next;
    int tag;
} cake_t;

cake_t* cakes;
pthread_mutex_t cakes_mutex;
pthread_cond_t cakes_cond;

void *producer(void *arg) {
    cake_t *new_cake;
    srand(time(NULL));
    while(1) {
        new_cake = (cake_t *)malloc(sizeof(cake_t));
        new_cake->tag = rand() % 100;
        printf("--< produce, tag = %d <--\n", new_cake->tag);
        pthread_mutex_lock(&cakes_mutex);
        new_cake->next = cakes;
        cakes = new_cake;
        pthread_mutex_unlock(&cakes_mutex);
        pthread_cond_signal(&cakes_cond);
        sleep(rand()%3);
    }
    return (void *)0;
}

void *consumer(void *arg) {
    cake_t *tmp_cake;
    srand(time(NULL));
    while(1) {
        pthread_mutex_lock(&cakes_mutex);
        while(cakes == NULL) {
            pthread_cond_wait(&cakes_cond, &cakes_mutex);
        }
        tmp_cake = cakes;
        cakes = cakes->next;
        pthread_mutex_unlock(&cakes_mutex);
        printf("--> consume, tag = %d >--\n", tmp_cake->tag);
        free(tmp_cake);
        sleep(rand() % 3);
    }
    return (void *)0;
}

int main(void) {
    pthread_t p_tid, c_tid1, c_tid2;
    cakes = NULL;
    srand(time(NULL));
    pthread_cond_init(&cakes_cond, NULL);

    pthread_create(&p_tid, NULL, producer, NULL);
    pthread_create(&c_tid1, NULL, consumer, NULL);
    pthread_create(&c_tid2, NULL, consumer, NULL);

    pthread_join(p_tid, NULL);
    pthread_join(c_tid1, NULL);
    pthread_join(c_tid2, NULL);
    exit(1);
}