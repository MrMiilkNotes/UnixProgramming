#include "epoll_func.h"
#include "../include/apue.h"

int Epoll_create(int size) {
  int ret;
  if ((ret = epoll_create(size)) == -1) {
    err_sys("epoll_create error");
    exit(1);
  }
  return ret;
}

int Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
  int ret;
  if ((ret = epoll_ctl(epfd, op, fd, event)) == -1) {
    err_sys("epoll_ctl error");
    exit(1);
  }
  return ret;
}

int Epoll_wait(int epfd, struct epoll_event *event, int maxevents,
               int timeout) {
  int ret;
  if ((ret = epoll_wait(epfd, event, maxevents, timeout)) == -1) {
    err_sys("epoll_ctl error");
    exit(1);
  }
  return ret;
}