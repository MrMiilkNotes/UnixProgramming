/*
 *  epoll实现的多路IO转接 -- 反应堆模型
 *  - 之前的模型中事件触发后返回了可以进行IO的文件描述符，
 *      但是强大的epoll在epoll_event中支持了泛型指针，因此我们可以
 *      自定义自己的返回结构体，携带自己需要的信息，甚至包括处理的
 *      函数指针
 *  - socket初始化部分分离到initSocket
 *  - 反应堆模型：
 *      - _event_data为事件返回的结构体类型，
 *          实现event_*便于结构体的初始化以及设置，修改，删除事件
 *                                           => 之后整理为单独的.h(TODO)
 *      - 事件的处理函数
 *          - conn_handler  - 新连接
 *          - read_handler  - 读事件
 *          - write_handler - 写事件
 *
 */
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include "../include/apue.h"
#include "epoll_func.h"
#include "sock_func.h"

#define PROTOCOL AF_INET
#define SERVER_IP INADDR_ANY
#define SERVER_PORT 8080
#define LISTEN_BACKLOG 128
#define EPOLL_INITSIZE 10
#define EPOLL_RET_SIZE EPOLL_INITSIZE
#define BUFFERSIZE BUFSIZ

// --------------------------------------------------------------------------------
//      初始化socket连接
// --------------------------------------------------------------------------------
int initSocket(int epfd, event_data_t *ev);
// --------------------------------------------------------------------------------
//      反应堆模型相关结构体与函数
// --------------------------------------------------------------------------------
typedef struct _event_data {
  int status;                    // 记录节点状态：
                                 //     -1：未使用
                                 //     0：经过初始化
                                 //     1：已经记录在epfd
  int event;                     // 记录对应的监听类型
  int sockfd;                    // 监听的sock
  void *arg;                     // 指向本身的指针，用于传入handler
  void (*handler)(int, void *);  // 事件满足的处理函数
  char buf[BUFFERSIZE];          // 记录返回给client的数据
  size_t len;                    // 数据长度
  time_t last_active;  // client上次活跃时间，用于清除长时间连接的不活跃用户
} event_data_t;

// 设置回调函数
void event_set(int sockfd, void (*handler)(int, void *), event_data_t *ev);
// 添加到epoll监听
void event_add(int epfd, int event, event_data_t *ev);
// 修改epoll监听
void event_mod(int epfd, int event, void (*handler)(int, void *),
               event_data_t *ev);
// 从epoll监听中删除
void event_del(int epfd, event_data_t *ev);
// 新连接
void conn_handler(int server_sockfd, void *arg);
// 读事件
void read_handler(int sockfd, void *arg);
// 写事件
void write_handler(int sockfd, void *arg);

// --------------------------------------------------------------------------------
//      global variable
//          epfd epoll文件描述符
//          clients 记录client连接，即epoll返回的指针指向的内容
//              长度+1是为了将server自己的socketfd记录在最后一个
// --------------------------------------------------------------------------------
int epfd;
event_data_t clients[EPOLL_INITSIZE + 1];

// ================================================================================
//      main
// ================================================================================
int main(void) {
  int i, nready;
  event_data_t *ret_event;  // 用于循环处理每个事件
  struct epoll_event
      ret_events[EPOLL_INITSIZE + 1];  // epoll返回的事件结构体数组

  // ---------------------------------------
  // initialization
  // ---------------------------------------
  epfd = Epoll_create(EPOLL_INITSIZE);  // epoll初始化
  for (i = 0; i <= EPOLL_INITSIZE; ++i) {
    clients[i].status = -1;  // 用 status=-1 表示未使用
  }
  // 这里将server自己的socketfd记录在clients的最后以为(!!!!)
  initSocket(epfd, &clients[EPOLL_INITSIZE]);  // 设置socket

  // ---------------------------------------
  // 循环处理，从返回列表中获取事件后直接调用
  // _event_data中的callback函数处理
  //    从read中获取到的内容会以字符串形式保存到
  //    _event_data中，直到之后write。 => 这样可能会有不合理之处(TODO)
  // ---------------------------------------
  while (1) {
    nready = Epoll_wait(epfd, ret_events, EPOLL_INITSIZE + 1, -1);
    for (i = 0; i < nready; ++i) {
      ret_event = ret_events[i].data.ptr;
      ret_event->handler(ret_event->sockfd, ret_event->arg);
    }
  }
  return 0;
}
// ================================================================================
//      end
// ================================================================================

void event_set(int sockfd, void (*handler)(int, void *), event_data_t *ev) {
  ev->status = 0;
  ev->sockfd = sockfd;
  ev->arg = ev;  // 记录本身
  ev->handler = handler;
  bzero(ev->buf, BUFFERSIZE);
  ev->len = 0;
  ev->last_active = time(NULL);
}

void event_add(int epfd, int event, event_data_t *ev) {
  int op;
  struct epoll_event ep_event;

  // TODO：处理 未经过set的event直接add
  if (ev->status == 0) {
    op = EPOLL_CTL_ADD;
    ev->status = 1;
  } else if (ev->status == 1) {
    op = EPOLL_CTL_MOD;
  }
  ep_event.events = ev->event = event;
  ep_event.data.ptr = ev;
  ev->last_active = time(NULL);
  Epoll_ctl(epfd, op, ev->sockfd, &ep_event);
  // (!!!) 这里传入&ep_event后，ep_event会被释放，因此可以看到相应的结构体数据被
  // 复制进内核了，并非只是一个指针
}

void event_mod(int epfd, int event, void (*handler)(int, void *),
               event_data_t *ev) {
  struct epoll_event ep_event;
  ev->handler = handler;
  ep_event.events = ev->event = event;
  ep_event.data.ptr = ev;
  ev->last_active = time(NULL);
  Epoll_ctl(epfd, EPOLL_CTL_MOD, ev->sockfd, &ep_event);
}

void event_del(int epfd, event_data_t *ev) {
  if (ev->status == -1) return;
  Epoll_ctl(epfd, EPOLL_CTL_DEL, ev->sockfd, NULL);
  ev->status = -1;
}

int initSocket(int epfd, event_data_t *ev) {
  int opt = 1;  // 端口复用
  struct sockaddr_in server_addr;
  int server_sockfd;
  int flag;

  // ---------------------------------------
  // initialization
  // ---------------------------------------
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = PROTOCOL;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = htonl(SERVER_IP);
  bzero(ev, sizeof(ev));
  // ---------------------------------------
  // socket and epoll settings
  // ---------------------------------------
  server_sockfd = Socket(PROTOCOL, SOCK_STREAM, 0);  // TCP
  // 设置非阻塞IO
  flag = fcntl(server_sockfd, F_GETFL);
  flag |= O_NONBLOCK;
  if (fcntl(server_sockfd, F_SETFL, flag) == -1) {
    err_sys("server_sockfd, fcntl error");
    exit(1);
  }
  // 设置端口复用
  setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  // 设置socket
  Bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  Listen(server_sockfd, LISTEN_BACKLOG);
  // 设置epoll监听事件
  event_set(server_sockfd, conn_handler, ev);
  event_add(epfd, EPOLLIN, ev);

  return server_sockfd;
}

void conn_handler(int server_sockfd, void *arg) {
  event_data_t *event_data_ptr = (event_data_t *)arg;
  int client_sockfd;
  struct sockaddr_in client_addr;
  int client_addr_len;
  char cli_ip[BUFFERSIZE];
  int i;
  event_data_t *ev_ptr;
  int flag;  // 非阻塞IO

  client_sockfd =
      Accept(server_sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
  event_data_ptr->last_active = time(NULL);  // 更新活动时间
  printf("received connection ... \nip\t:%s\nport\t:%d\n",
         inet_ntop(PROTOCOL, &(client_addr.sin_addr.s_addr), cli_ip,
                   sizeof(cli_ip)),
         ntohs(client_addr.sin_port));
  // 将连接记录到clients中，其中status == -1为未使用的存储位置
  for (i = 0; i < EPOLL_INITSIZE; ++i) {
    if (clients[i].status == -1) {
      ev_ptr = clients + i;
      break;
    }
  }
  do {
    // 达到了epoll设置的事件上限
    // 这里直接关闭了新连接(TODO)
    if (i == EPOLL_INITSIZE) {
      printf("too many connection ... closing ...\n");
      shutdown(client_sockfd, SHUT_RDWR);
      break;
    }
    // 设置非阻塞IO
    flag = fcntl(client_sockfd, F_GETFL);
    flag |= O_NONBLOCK;
    if (fcntl(client_sockfd, F_SETFL, flag) == -1) {
      err_sys("fcntl error");
      shutdown(client_sockfd, SHUT_RDWR);
      break;
    }
    // 设置epoll监听事件
    event_set(client_sockfd, read_handler, ev_ptr);
    event_add(epfd, EPOLLIN, ev_ptr);
  } while (0);
}

void read_handler(int sockfd, void *arg) {
  event_data_t *event_data_ptr = (event_data_t *)arg;
  char buf[BUFFERSIZE];
  ssize_t sz;
_read_again:
  // 这里buf和event_data_ptr->buf都是BUFFERSIZE大小
  // 因此只读取一次，如果信息很长可以不断读取，然后拼接(TODO)
  if ((sz = read(sockfd, buf, BUFFERSIZE)) <= 0) {
    if (sz < 0 && errno != EAGAIN) {
      // 中断，可重入
      if (errno == EINTR) {
        goto _read_again;
      }
      err_sys("read from socket error");
    } else if (sz == 0) {
      // client连接关闭 ==> 如果client连接关闭，epoll会以可读条件返回(!!!!!)
      printf("socket closed by client");
    }
    shutdown(sockfd, SHUT_RD);
    event_del(epfd, event_data_ptr);
  } else if (errno != EAGAIN) {
    strncpy(event_data_ptr->buf, buf, sz);
    event_data_ptr->len = sz;
    event_mod(epfd, EPOLLOUT, write_handler, event_data_ptr);
  }
  // else errno == EAGAIN
  // 本次数据已经读取结束 -> 使用循环读取的时候应该注意EAGAIN
}

void write_handler(int sockfd, void *arg) {
  event_data_t *event_data_ptr = (event_data_t *)arg;
  ssize_t sz;
_write_again:
  if ((sz = write(sockfd, event_data_ptr->buf, event_data_ptr->len)) !=
      event_data_ptr->len) {
    // (!!TODO)这里需要测试如果写入的时候发生
    // errno ==EINTR 到底会处于一种什么状态
    // 写入失败，等价没有数据写入，或者写入了一部分数据
    if (sz == -1 && (errno == EINTR)) {
      goto _write_again;
    }
    err_sys("socket write error");
    shutdown(sockfd, SHUT_RD);
    event_del(epfd, event_data_ptr);
  } else {
    event_mod(epfd, EPOLLIN, read_handler, event_data_ptr);
  }
}