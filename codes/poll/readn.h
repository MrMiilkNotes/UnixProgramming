/*
 * 提供对socket的读取一行操作
 */

#ifndef READN_H
#define READN_H

#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "../include/apue.h"

ssize_t sock_readline(int sockfd, void *buf, size_t maxlen);
ssize_t readn(int sockfd, void *buf, size_t n);
#endif