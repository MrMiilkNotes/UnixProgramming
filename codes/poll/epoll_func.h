#ifndef EPOLL_FUNC_H
#define EPOLL_FUNC_H

#include <sys/epoll.h>

// epoll_create 的封装，出错会直接exit(1);
int Epoll_create(int size);
int Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
// epoll_wait 的封装，出错会直接exit(1);
int Epoll_wait(int epfd, struct epoll_event *event, int maxevents, int timeout);
#endif
