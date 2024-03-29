// wait 在子进程几种出错情况下的表现
#include <sys/wait.h>
#include "../include/apue.h"

void pr_exit(int status);

int main(int argc, char const *argv[]) {
  pid_t pid;
  int status;

  if ((pid = fork()) < 0)
    err_sys("fork error");
  else if (pid == 0)
    exit(7);

  if (wait(&status) != pid) err_sys("wait error");
  pr_exit(status);

  if ((pid = fork()) < 0)
    err_sys("fork error");
  else if (pid == 0)
    abort();

  if (wait(&status) != pid) err_sys("wait error");
  pr_exit(status);

  if ((pid = fork()) < 0)
    err_sys("fork error");
  else if (pid == 0)
    status /= 0;

  if (wait(&status) != pid) err_sys("wait error");
  pr_exit(status);

  exit(0);
}
