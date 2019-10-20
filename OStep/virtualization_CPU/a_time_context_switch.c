// ----------------------------------------
// 测量一次上下文切换用时
// ----------------------------------------
// #define _GNU_SOURCE
// #include <sched.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include "../apue.h"

#define BUFSIZE 10

int main() {
  //   cpu_set_t cpuset;
  //   int num_cpu = 2;

  //   CPU_ZERO(&cpuset);
  //   CPU_SET(1, &cpuset);

  //   // 绑定到一个CPU
  //   if (sched_getaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {
  //     err_sys("sched_getaffinity error");
  //     exit(1);
  //   }

  // 然鹅阿里云上的CPU的确只有一个逻辑处理器。。。
  struct timeval t_start, t_end;
  int pipe1[2], pipe2[2];
  // pipe1 父进程写 子进程读
  // pipe2 父进程读 子进程写
  pid_t pid;
  char buf[BUFSIZE], msg = 'a';
  int i, n_iter = 2 << 15;
  float pad;

  if (pipe(pipe1) < 0) {
    err_sys("init pipe error");
    exit(1);
  }
  if (pipe(pipe2) < 0) {
    err_sys("init pipe error");
    exit(1);
  }

  if ((pid = fork()) < 0) {
    err_sys("fork error");
    exit(0);
  } else if (pid == 0) {
    // child
    close(pipe1[1]);
    close(pipe2[0]);
    sleep(3);
    while (1) {
      read(pipe1[0], buf, BUFSIZE);
      write(pipe2[1], &msg, sizeof(msg));
    }
    close(pipe1[0]);
    close(pipe2[1]);
  } else {
    // parent
    close(pipe1[0]);
    close(pipe2[1]);
    i = 0;
    sleep(1);
    if (gettimeofday(&t_start, NULL) != 0) {
      err_sys("gettimeofday error");
      exit(1);
    }
    for (; i < n_iter; ++i) {
      write(pipe1[1], &msg, sizeof(msg));
      read(pipe2[0], buf, BUFSIZE);
    }
    close(pipe1[1]);
    close(pipe2[0]);
  }
  if (gettimeofday(&t_end, NULL) != 0) {
    err_sys("gettimeofday error");
    exit(1);
  }

  printf("done\n");

  printf("start time: %lu: %lu\n", t_start.tv_sec, t_start.tv_usec);
  printf("end time: %lu: %lu\n", t_end.tv_sec, t_end.tv_usec);
  pad = (t_end.tv_sec - t_start.tv_sec) * 1000000.0 + t_end.tv_usec -
        t_start.tv_usec;
  printf("measure with %d circle, %f usec per context switch in this process\n", n_iter, pad / n_iter);
  return 0;
}