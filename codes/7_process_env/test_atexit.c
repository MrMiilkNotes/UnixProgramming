#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void bye(void)
{
    printf("before bye\n");
    sleep(10);
    printf("after bye\n");
}

void *do_thread(void)
{
    while (1)
    {
        printf("--------------I'm thread!\n");
        sleep(1);
    }
}

int main(void)
{
    pthread_t pid_t;

    atexit(bye);
    pthread_create(&pid_t, NULL, (void *)do_thread, NULL);

    exit(EXIT_SUCCESS);
}