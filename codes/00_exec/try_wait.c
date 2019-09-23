#include <sys/wait.h>
#include <time.h>
#include "../include/apue.h"

void pr_exit(int status) {
  if (WIFEXITED(status)) {
    printf("normal termination, exit status: %d\n", WEXITSTATUS(status));
  } else if (WIFSIGNALED(status)) {
    printf("abnormal termation, signal number = %d\n", WTERMSIG(status));
  }
}

int main(void) {
  pid_t pid;
  int status;

  if ((pid = fork()) < 0) {
    err_sys("fork error");
  } else if (pid == 0) {
    // child
    sleep(20);
    int k = 1;
    // k /= 0;
  }

  wait(&status);
  pr_exit(status);

  exit(0);
}