#include "./readn.h"
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "../include/apue.h"

ssize_t __readn_(int fd, void *buf, size_t len) {
  int ret;
__readn_again:
  if ((ret = read(fd, buf, len)) < 0) {
    if (errno == EINTR) {
      goto __readn_again;
    } else {
      return -1;
    }
  }
  return len;
}

ssize_t __recv_peek_(int sockfd, void *buf, size_t len) {
  int ret;
__recv_peek_again:
  if ((ret = recv(sockfd, buf, len, MSG_PEEK)) == -1) {
    if (errno == EINTR) {
      goto __recv_peek_again;
    } else {
      // TODO: errno
      return -1;
    }
  }
  return ret;
}

ssize_t sock_readline(int sockfd, void *buf, size_t maxlen) {
  int ret, count, nread;
  int nleft = maxlen;
  char *buf_ptr = buf;
  int i;

  count = 0;
  while (1) {
    if ((ret = __recv_peek_(sockfd, buf_ptr, nleft)) < 0) {
      return -1;
    } else if (ret == 0) {
      // socket closed, return 0;(!!!!!!)
      return ret;
    }
    // search for '\n'
    nread = ret;
    for (i = 0; i < nread; ++i) {
      if (buf_ptr[i] == '\n') {
        if ((ret = __readn_(sockfd, buf, i + 1)) != i + 1) {
          return ret;
        }
        return ret + count;
      }
    }
    if (nleft < nread) {
      // maxlen 装不下一行
      // TODO: errno
      return -1;
    }
    nleft -= nread;
    if ((ret = __readn_(sockfd, buf_ptr, nread)) != nread) {
      return -1;
    }
    buf_ptr += nread;
    count += nread;
  }
}