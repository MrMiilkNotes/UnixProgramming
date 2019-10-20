/*
 * epoll实现的多路IO转接
 *  epoll的机制和select会有所不同，但是策略是一样的，让epoll返回的是指针
 *  初始化epoll，然后填写相应的注册结构体进行注册，然后使用epoll_wait等待
 *  更为复杂的注册也对应了epoll返回更多的事件信息，在注册的epoll_event中有泛型指针，
 *  因此可以记录几乎所有需要的信息
 */

#include <arpa/inet.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "../include/apue.h"
#include "epoll_func.h"
#include "readn.h"
#include "sock_func.h"

#define PROTOCOL AF_INET
#define SERVER_IP INADDR_ANY
#define SERVER_PORT 8080
#define LISTEN_BACKLOG 128
#define EPOLL_INITSIZE 10  // epoll初始化监听的文件描述符数量
#define EPOLL_RET_SIZE EPOLL_INITSIZE  // epoll_wait的数量
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
  int epoll_fd;                     // epoll本身是用文件描述符指示的
  struct epoll_event listen_event;  // 用于向epoll添加事件
  struct epoll_event ret_events[EPOLL_RET_SIZE];
  int i, nready;
  // >>> epoll

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
  /* --------------------------------------------------------------------------
   *   同样是循环等待连接请求
   * --------------------------------------------------------------------------
   */
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
          bzero(&listen_event, sizeof(listen_event));
          listen_event.events = EPOLLIN;
          listen_event.data.fd = client_sockfd;
          Epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sockfd, &listen_event);
        } else {
          /* ----------------------------------------------------------------
           * client: read event
           * 这里如果使用非阻塞IO，需要对返回值进行更仔细的判断，不能使用readn函数
           * 对于非阻塞IO，read返回后errno=EAGIN，说明本次数据已经读取完成，应该
           * 继续处理下一个请求，可以看server_noblock.c。
           * 另外，非阻塞IO可以在接收到新连接请求后用fctl设置
           * ----------------------------------------------------------------
           */
          if ((sz = sock_readline(ret_events[i].data.fd, msg, BUFFERSIZE)) <=
              0) {
            if (sz < 0) {
              err_sys("read error");
            }
            // close client connection
            printf("close client ...\n");
            Epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ret_events[i].data.fd, NULL);
            shutdown(ret_events[i].data.fd, SHUT_RDWR);
          } else {
            printf("received: '%s'\n", msg);
            // 这里只注册了读事件，可读后会直接将结果写入socket
            // TODO: 应该将写事件加入epoll
            if ((sz = write(ret_events[i].data.fd, msg, sz)) != sz) {
              printf("write error\n");
            }
          }
        }
      }
    }
  }
}