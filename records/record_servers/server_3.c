/*
 * 多路IO转接模型 -- 使用 select
 *  - 从单进程单次处理到多进程，之前有听过进程池的概念，像是数据库也有这种概念，
 *      这里还有一个很好的机制，多路IO转接。
 *  - 之前的处理框架是用server接受连接请求，再fork子进程进行处理。
 *  -
 * 从之前单进程服务器出发，如果想处理多个客户的请求，那就得轮询多个client的连接，
 *      这种方式在效率上不行，首先是询问到的client不一定准备好，其次是不断询问IO本身就是很耗费时间的行为
 *  -
 * 设想一种机制：client的socket能进行读写的时候有类似信号的通知，从而server能直接提供服务。内核其实是有提供这种
 *      服务的，从select到poll，再到著名的epoll
 *
 *  这里使用的是select。它使用了fd_set结构（位图）来记录监听的文件描述符，
 *  调用select进入阻塞，当有描述符准备好读写的时候就会返回
 */
#include <arpa/inet.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "../include/apue.h"
#include "readn.h"
#include "sock_func.h"

#define SERVER_IP INADDR_ANY
#define SERVER_PORT 8080
#define PROTOCOL AF_INET
#define BUFFERSIZE BUFSIZ
#define LISTEN_BACKLOG 128

int main(void) {
  // socket
  struct sockaddr_in server_addr, client_addr;
  int server_sockfd, client_sockfd;
  int i, fdset[FD_SETSIZE], client_addr_sz;  // i用于迭代
                                             // fdset 记录client的sockfd
                                             // client_addr_sz 为accept参数
  ssize_t sz;
  char cli_ip[BUFFERSIZE], msg[BUFFERSIZE];
  // select
  fd_set read_set, readable_set;
  int nready, maxfds;  // maxfds 记录最大的描述符值，用于select
                       // nready 用于记录select返回值，有多少个连接准备好
  // 使用fdset来记录client的socket文件描述符
  // 用于select返回后查询 ==> 这里体现出select一些不足之处
  for (i = 0; i < FD_SETSIZE; ++i) {
    fdset[i] = -1;
  }
  FD_ZERO(&read_set);
  maxfds = -1;

  // ------------------------------------------------------
  // socket初始化
  // ------------------------------------------------------
  server_sockfd = Socket(PROTOCOL, SOCK_STREAM, 0);
  maxfds = server_sockfd;
  // TODO: 端口复用
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = PROTOCOL;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = htonl(SERVER_IP);
  Bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  Listen(server_sockfd, LISTEN_BACKLOG);

  // server_sockfd 不作为客户对象，但是属于需要监听的对象
  FD_SET(server_sockfd, &read_set);
  // ------------------------------------------------------
  //      处理循环
  // ------------------------------------------------------
  while (1) {
    /*
     *  这里请特别注意，当select返回的时候填写的readable_set是可以进行IO（这里只是读）的文件描述符集合
     *  而read_set是server监听的所有文件描述符集合
     */
    readable_set = read_set;
    nready = select(maxfds + 1, &readable_set, NULL, NULL, NULL);

    /* ------------------------------------------------------
     * server_sockfd --> 新的连接请求
     * ------------------------------------------------------
     */
    if (FD_ISSET(server_sockfd, &readable_set)) {
      --nready;
      bzero(&client_addr, sizeof(client_addr));
      client_sockfd = Accept(server_sockfd, (struct sockaddr *)&client_addr,
                             &client_addr_sz);
      printf("received connection ... \nip\t:%s\nport\t:%d\n",
             inet_ntop(AF_INET, &(client_addr.sin_addr.s_addr), cli_ip,
                       sizeof(cli_ip)),
             ntohs(client_addr.sin_port));
      // 将新的客户端连接记录到read_set
      for (i = 0; i < FD_SETSIZE; ++i) {
        if (fdset[i] == -1) {
          fdset[i] = client_sockfd;
          maxfds = maxfds > client_sockfd ? maxfds : client_sockfd;
          FD_SET(client_sockfd, &read_set);
          break;
        }
      }
      // select的不足之一，限制打开的文件描述符数量，可以通过修改并重新编译系统更改
      if (i == FD_SETSIZE) {
        printf("too many socket conection, closing client: %s\n", cli_ip);
        close(client_sockfd);
      }
    }
    /* ------------------------------------------------------
     * 其他socketfd --> 有客户已经有数据可读
     *   单进程处理请求。
     *   这里同时也可以看到select的一个不足：需要遍历所有文件描述符进行查询。
     *   之后到epoll的时候才不用遍历
     * ------------------------------------------------------
     */
    for (i = 0; i < FD_SETSIZE; ++i) {
      if (nready <= 0) break;
      if (fdset[i] != -1 && FD_ISSET(fdset[i], &readable_set)) {
        nready--;
        client_sockfd = fdset[i];
        // 对request的同样处理
        if ((sz = sock_readline(client_sockfd, msg, BUFFERSIZE)) != 0) {
          if (sz < 0) {
            err_sys("read error");
          } else {
            printf("received: '%s'\n", msg);
            if ((sz = write(client_sockfd, msg, sz)) != sz) {
              printf("write error\n");
            }
          }
        } else {
          // close client connection
          printf("closing client ...\n");
          shutdown(client_sockfd, SHUT_RDWR);
          FD_CLR(client_sockfd, &read_set);
          fdset[client_sockfd] = -1;
        }
      }
    }
  }
  // close or shutdown
  close(server_sockfd);
  return 0;
}