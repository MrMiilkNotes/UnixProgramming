/*
 * epoll
 */
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "../include/apue.h"
#include "epoll_func.h"
#include "readn.h"
#include "sock_func.h"

#define PROTOCOL AF_INET
#define SERVER_IP INADDR_ANY
#define SERVER_PORT 8080
#define LISTEN_BACKLOG 128
#define EPOLL_INITSIZE 10
#define EPOLL_RET_SIZE EPOLL_INITSIZE
#define BUFFERSIZE BUFSIZ

int main(void) {
  // <<< socket
  int opt = 1;
  struct sockaddr_in server_addr, client_addr;
  int server_sockfd, client_sockfd;
  int client_addr_len;
  char cli_ip[BUFFERSIZE], msg[BUFFERSIZE];
  ssize_t sz;
  // >>> socket
  // <<< epoll
  int epoll_fd;
  struct epoll_event listen_event, ret_event;
  struct epoll_event ret_events[EPOLL_RET_SIZE];
  int i, nready;
  // >>> epoll
  // <<< noblock IO
  int flag;
  // >>> noblock IO

  // ---------------------------------------
  // initialization
  // ---------------------------------------
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = PROTOCOL;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = htonl(SERVER_IP);

  bzero(&listen_event, sizeof(listen_event));
  epoll_fd = Epoll_create(EPOLL_INITSIZE);

  // ---------------------------------------
  // create socket
  // ---------------------------------------
  server_sockfd = Socket(PROTOCOL, SOCK_STREAM, 0);
  setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  Bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  Listen(server_sockfd, LISTEN_BACKLOG);

  // ---------------------------------------
  // epoll: add server fd, and wait
  // ---------------------------------------
  listen_event.events = EPOLLIN;
  listen_event.data.fd = server_sockfd;
  Epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sockfd, &listen_event);
  while (1) {
    nready = Epoll_wait(epoll_fd, ret_events, EPOLL_RET_SIZE, -1);
    if (nready == 0) {
      // TODO
    } else {
      for (i = 0; i < nready; ++i) {
        if (!ret_events[i].events & EPOLLIN) {
          continue;
        }
        if (ret_events[i].data.fd == server_sockfd) {
          // ---------------------------------------
          // socket: new connection
          // ---------------------------------------
          client_sockfd = Accept(server_sockfd, (struct sockaddr *)&client_addr,
                                 &client_addr_len);
          printf("received connection ... \nip\t:%s\nport\t:%d\n",
                 inet_ntop(AF_INET, &(client_addr.sin_addr.s_addr), cli_ip,
                           sizeof(cli_ip)),
                 ntohs(client_addr.sin_port));
          // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ noblock ~~~~~~~~~~~~~~~~~~~~~~~~~~~
          flag = fcntl(client_sockfd, F_GETFL);
          flag |= O_NONBLOCK;
          fcntl(client_sockfd, F_SETFL, flag);
          // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
          // add to epfd
          bzero(&listen_event, sizeof(listen_event));
          listen_event.events = EPOLLIN;
          listen_event.data.fd = client_sockfd;
          Epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sockfd, &listen_event);
        } else {
          // ---------------------------------------
          // client: read event
          // 非阻塞IO，需要对返回值进行更仔细的判断，不能使用readn函数
          // ---------------------------------------
          while ((sz = read(ret_events[i].data.fd, msg, BUFFERSIZE)) > 0) {
            printf("received: '%s'\n", msg);
            // TODO: writable???
            if ((sz = write(ret_events[i].data.fd, msg, sz)) != sz) {
              printf("write error\n");
            }
          }
          if (sz <= 0) {
            if (sz < 0) {
              if (errno == EAGAIN) {
                // 非阻塞IO
                continue;
              }
              err_sys("read error");
            }
            // close client' connection
            printf("close client ...\n");
            Epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ret_events[i].data.fd, NULL);
            shutdown(ret_events[i].data.fd, SHUT_RDWR);
          }
        }
      }
    }
  }
}