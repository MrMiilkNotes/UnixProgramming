#include"../include/apue.h"
#include<pthread.h>
    
pthread_mutex_t mutex; 

void *thr_fn(void *arg) {
    srand(time(NULL));
    while(1) {
        pthread_mutex_lock(&mutex);
        printf("HELLO WORLD\n");
        sleep(rand() % 3);
        printf("END\n");
        pthread_mutex_unlock(&mutex);

        sleep(rand() % 3);
    }
}

int main(void) {
    pthread_t tid;

    srand(time(NULL));
    pthread_mutex_init(&mutex, NULL);

    pthread_create(&tid, NULL, thr_fn, NULL);
    while(1) {
        pthread_mutex_lock(&mutex);
        printf("hello world\n");
        sleep(rand() % 3);
        printf("end\n");
        pthread_mutex_unlock(&mutex);

        sleep(rand() % 3);
    }

    pthread_mutex_destroy(&mutex);
    exit(0);
}