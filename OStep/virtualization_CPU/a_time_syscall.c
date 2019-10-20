// ----------------------------------------
// 测量一次系统调用用时
// ----------------------------------------
#include <sys/time.h>
#include <unistd.h>
#include "../apue.h"

int main(void) {
  struct timeval t_start, t_end;
  int i, n_it = 2 << 20;
  float pad;
  char buf[1];

  if (gettimeofday(&t_start, NULL) != 0) {
    err_sys("gettimeofday error");
    exit(1);
  }

  for (i = 0; i < n_it; ++i) {
    if (read(STDIN_FILENO, buf, 0) == -1) {
      err_sys("read error");
      exit(1);
    }
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
  printf("measure with %d circle, %f usec per syscall\n", n_it, pad / n_it);
  return 0;
}