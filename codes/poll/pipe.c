#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "../include/apue.h"
#include "epoll_func.h"
// epoll 这些函数不只是用于socket，其他文件描述符都是类似的

int main(void) {
  pid_t pid;
  int fds[2];  // 0: read; 1: write
  int epoll_sz = 10;
  int epfd;
  int status;

  // initialization
  if ((status = pipe(fds)) < 0) {
    err_sys("init pipe error");
  }
  epfd = Epoll_create(epoll_sz);

  if ((pid = fork()) < 0) {
    err_sys("fork error");
    exit(1);
  } else if (pid == 0) {
    // child
    close(fds[0]);
  } else {
    // parent
    close(fds[1]);
  }
}
