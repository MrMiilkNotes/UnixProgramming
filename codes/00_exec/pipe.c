#include <string.h>
#include <unistd.h>
#include "../include/apue.h"

int main(void) {
  int fd[2];
  pid_t pid;
  ssize_t nsend;
  int status;

  if ((status = pipe(fd)) < 0) {
    err_sys("init pipe error");
  }

  if ((pid = fork()) < 0) {
    err_sys("fork error");
  } else if (pid == 0) {
    // child
    close(fd[0]);
    const char *msg = "hello, parent\n";
    nsend = write(fd[1], msg, strlen(msg));
  } else {
    close(fd[1]);
    char buf[1024];
    nsend = read(fd[0], buf, sizeof(buf));
    if (nsend == 0) {
      printf("-------\n");
    }
    write(STDOUT_FILENO, buf, nsend);
  }

  exit(0);
}