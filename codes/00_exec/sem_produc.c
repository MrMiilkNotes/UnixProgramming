#include"../include/apue.h"
#include<pthread.h>
#include<semaphore.h>

#define NUM_CAKE 10

sem_t table_sem;
sem_t cakes_sem;

int cakes[NUM_CAKE];
int count;

void *producer(void *arg) {
    srand(time(NULL));
    while(1) {
        sem_wait(&table_sem);
        cakes[(count++) % NUM_CAKE] = count;
        sem_post(&cakes_sem);
    }
}

void *consumer(void *arg) {
    while(1) {
        sem_wait(&cakes_sem);
        printf("---- consum cake: %d\n", cakes[])
    }
}


int main(void) {
    sem_init(&table_sem, 0, NUM_CAKE);
    sem_init(&cakes_sem, 0, 0);
    count = 0;
    
}