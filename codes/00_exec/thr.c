#include <pthread.h>
#include "../include/apue.h"

void *thr_fn(void *arg) {
  printf("in thread: %d, tid =%lu \n", (int)arg, pthread_self());

  return (void *)0;
}

int main() {
  pthread_t tid;
  int i, N = 10;
  int errnum;

  for (i = 0; i < N; ++i) {
    if ((errnum = pthread_create(&tid, NULL, thr_fn, (void *)(i + 1)))) {
      err_sys("create thread %d : %s\n", i + 1, strerror(errnum));
    }
  }

  printf("in main, tid = %lu\n", pthread_self());

  // wait ...
  sleep(1);
  exit(0);
}