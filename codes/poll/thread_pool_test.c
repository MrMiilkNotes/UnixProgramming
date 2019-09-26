#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #include<unistd.h>
#include "thread_pool.h"

struct msg {
  size_t sz;
  char buf[BUFSIZ];
};

void process(void *arg) {
  char buf[BUFSIZ];
  struct msg *msg_ptr = (struct msg *)arg;
  sprintf(buf, "Received msg: %s\n", msg_ptr->buf);
  sleep(time(NULL) % 5);
  printf("%s", buf);
  //   free(msg_ptr);
}

int main(void) {
  int i;
  int n_tasks = 50;
  thread_pool_t *t_pool_ptr;
  struct msg *msg_ptr;

  if ((t_pool_ptr = pool_create(POOL_INIT_SIZE, POOL_SIZE_MIN, POOL_SIZE_MAX,
                                POOL_TASK_QUEUE_SIZE)) == NULL) {
    printf("create thread pool error\n");
  }
  for (i = 0; i < n_tasks; ++i) {
    msg_ptr = (struct msg *)malloc(sizeof(struct msg));
    sprintf(msg_ptr->buf, "a sorted num: %d", i + 1);
    msg_ptr->sz = strlen(msg_ptr->buf);
    pool_add(t_pool_ptr, process, (void *)msg_ptr);
    sleep(time(NULL) % 2);
  }
  sleep(3);
  pool_destroy(t_pool_ptr);
}