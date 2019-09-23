#include <errno.h>
#include "../include/apue.h"

static void sig_hup(int signo) {
  printf("SIGHUP received, pid = %ld\n", (long)getpid());
}

static void pr_ids(char* name) {
  printf("%s: pid = %ld, ppid = %ld, pgrp = %ld, tpgrp = %ld\n", name,
         (long)getpid(), (long)getppid(), (long)getpgrp(),
         (long)tcgetpgrp(STDIN_FILENO));
  fflush(stdout);
}

int main(void) {
  char c;
  pid_t pid;

  pr_ids("parent");
  if ((pid = fork()) < 0) {
    err_sys("fork error");
  } else if (pid > 0) {
    sleep(5);
  } else {
    pr_ids("child");
    // 对SIGHUP的默认处理是终止，因此这里用函数捕捉
    signal(SIGHUP, sig_hup);
    // 将自己挂起
    fflush(stdout);
    kill(getpid(), SIGTSTP);
    // POSIX.1规定会给挂起的孤儿进程组中的进程发送SIGHUP
    // 后会发送SIGCONT信号
    pr_ids("child");
    fflush(stdout);
    if (read(STDIN_FILENO, &c, 1) != 1) {
      // 成为后台进程，尝试读终端输入，内核会产生SIGTTIN信号
      printf("read error %d on controlling TTY\n", errno);
    }
  }
  exit(0);
}