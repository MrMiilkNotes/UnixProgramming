#include "../include/apue.h"
// 捕捉自定义的信号

static void sig_usr(int);

int main(void) {
  if (signal(SIGUSR1, sig_usr) == SIG_ERR) {
    err_sys("cannot catch signal SIGUSR1");
  }
  if (signal(SIGUSR2, sig_usr) == SIG_ERR) {
    err_sys("cannot catch signal SIGUSR2");
  }

  for (;;) pause();
}

static void sig_usr(int signo) {
  if (signo == SIGUSR1) {
    printf("received SIGUSR1\n");
  } else if (signo == SIGUSR2) {
    printf("received SIGUSR2\n");
  } else {
    err_dump("received signal %d\n", signo);
  }
}