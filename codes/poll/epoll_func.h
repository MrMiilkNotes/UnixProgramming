#ifndef EPOLL_FUNC_H
#define EPOLL_FUNC_H

int Epoll_create(int size);
int Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int Epoll_wait(int epfd, struct epoll_event *event, int maxevents, int timeout);
#endif
