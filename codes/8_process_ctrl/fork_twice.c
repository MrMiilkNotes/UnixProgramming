// 使用两次fork来将子进程托管到init进程

#include <sys/wait.h>
#include <unistd.h>

#include "../include/apue.h"

int main(int argc, char const *argv[]) {
  int pid;

  if ((pid = fork()) < 0) err_sys("fork error");
  if (pid == 0) {
    // first child
    if ((pid = fork()) < 0) err_sys("fork error");
    if (pid > 0) {
      // first child
      exit(0);
    }
    // second child
    sleep(2);
    printf("second child, pid = %ld, ppid = %ld\n", (long)getpid(),
           (long)getppid());
    exit(0);
  }
  // parent
  if (waitpid(pid, NULL, 0) != pid) err_sys("waitpid error");
  exit(0);
  return 0;
}
