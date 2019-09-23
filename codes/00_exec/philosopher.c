#include"../include/apue.h"
#include<pthread.h>

#define NUM_PHILO 5
#define NUM_EAT 6

pthread_mutex_t chopsticks[NUM_PHILO];

void * philosopher(void *arg) {
    int tag = (int)arg;
    int stick_n1, stick_n2;
    int status;
    int count = NUM_EAT;
    srand(time(NULL));

    switch(tag) {
        case NUM_PHILO - 1:
            // last philosopher
            stick_n1 = 0;
            stick_n2 = tag;
            break;
        default:
            stick_n1 = tag;
            stick_n2 = tag + 1;
            break;
    }
    while(count--) {
        while(1) {
            pthread_mutex_lock(&chopsticks[stick_n1]);
            if((status = pthread_mutex_trylock(&chopsticks[stick_n2])) == 0) {
                break;
            } 
            // else if (status != EAGAIN) {
            //     printf("thread %d\t", tag + 1);
            //     err_sys("try lock error");
            //     exit(1);
            // }
            pthread_mutex_unlock(&chopsticks[stick_n1]);
            sleep(rand()%3);
        }
        printf("philosopher %d eating...\n", tag + 1);
        sleep(rand() % 5);
        printf("philosopher %d finished\n", tag + 1);
        pthread_mutex_unlock(&chopsticks[stick_n1]);
        pthread_mutex_unlock(&chopsticks[stick_n2]);
        sleep(rand()%3);
    }

    return (void *)0;
}

int main(void) {
    pthread_t philosophers[NUM_PHILO];
    int i;

    for(i = 0; i < NUM_PHILO; ++i) {
        if (pthread_mutex_init(&chopsticks[i], NULL) != 0) {
            err_sys("init mutex error");
            exit(1);
        }
    }

    for(i = 0 ; i < NUM_PHILO; ++i) {
        if (pthread_create(&philosophers[i], NULL, philosopher, (void *)i) != 0) {
            err_sys("create thread error");
            exit(1);
        }
        // if (pthread_detach(philosophers[i]) != 0) {
        //     err_sys("detach thread error");
        //     exit(1);
        // }
    }

    for(i = 0; i < NUM_PHILO; ++i) {
        pthread_join(philosophers[i], NULL);
    }


    for(i = 0; i < NUM_PHILO; ++i) {
        if (pthread_mutex_destroy(&chopsticks[i]) != 0) {
            err_sys("destroy mutex error");
            exit(1);
        }
    }
}