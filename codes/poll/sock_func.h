#ifndef SOCK_FUNC_H
#define SOCK_FUNC_H

#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "../include/apue.h"

int Socket(int domain, int type, int protocol);

int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int Listen(int sockfd, int backlog);

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

#endif