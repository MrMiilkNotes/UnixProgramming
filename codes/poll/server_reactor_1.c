#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "epoll_func.h"
#include "sock_func.h"
#include "thread_pool.h"

// server
#define PROTOCOL AF_INET
#define SERVER_IP INADDR_ANY
#define SERVER_PORT 8080
#define LISTEN_BACKLOG 128
#define BUFFERSIZE BUFSIZ
// epoll
#define EPOLL_INITSIZE 10
#define EPOLL_CLIENT_MAX 10
// client status
#define CLIENT_WEPOLL 1
#define CLIENT_THR_WAIT 2
#define CLIENT_THR_ACT 3
#define CLIENT_OFFLINE 4
// ----------------------------------------------
// client structure
// ----------------------------------------------
typedef struct _client {
  // TODO: mutex
  int sockfd;
  int event;   // 处理的事件类型标识
  int status;  // 用户状态，已挂到线程池，等待Epoll，已离线
  char msg[BUFFERSIZE];  // 回送信息
  size_t msg_len;        // 信息长度
  time_t last_act;       // 上次活跃时间
} client_t;

// ----------------------------------------------
// global variables
// ----------------------------------------------
int epfd;
client_t clients[EPOLL_CLIENT_MAX + 1];
// ----------------------------------------------
// functions
// ----------------------------------------------
// epfd - epoll; ep_server_ptr - server在epoll中注册的监听
int initSock(int epfd, client_t *ep_server_ptr);
int client_set(client_t *ep_server_ptr, int sockfd, int event);
int client_add(int epfd, int event, client_t *ptr);
int client_mod(int epfd_, int event, client_t *ptr, char *msg, size_t sz);
int client_del(int epfd, client_t *ptr);

// 处理从epoll返回的新连接
void connection_handler(int epfd, client_t *ret_cli);
// 线程池使用的函数，从socket读取
void read_handler(void *arg);
// 线程池使用的函数，写入socket
void write_handler(void *arg);

int main(void) {
  thread_pool_t *t_pool_ptr;
  client_t *ret_cli;
  int server_sockfd;
  struct epoll_event ret_events[EPOLL_CLIENT_MAX + 1];
  int i, nready;

  for (i = 0; i < EPOLL_CLIENT_MAX; ++i) {
    clients[i].status = -1;
  }

  if ((t_pool_ptr = pool_create(POOL_INIT_SIZE, POOL_SIZE_MIN, POOL_SIZE_MAX,
                                POOL_TASK_QUEUE_SIZE)) == NULL) {
    printf("thread pool create error\n");
    exit(1);
  }
  epfd = Epoll_create(EPOLL_INITSIZE);
  server_sockfd = initSock(epfd, &(clients[EPOLL_CLIENT_MAX]));

  while (1) {
    nready = Epoll_wait(epfd, ret_events, EPOLL_CLIENT_MAX + 1, -1);

    for (i = 0; i < nready; ++i) {
      printf("%d\n", ret_events[i].events);
      ret_cli = ret_events[i].data.ptr;
      switch (ret_events[i].events) {
        case EPOLLIN:
          if (ret_cli->sockfd == server_sockfd) {
            // new connection
            connection_handler(epfd, ret_cli);
          } else {
            // read event
            pool_add(t_pool_ptr, read_handler, (void *)ret_cli);
          }
          break;
        case EPOLLOUT:
          // write event
          pool_add(t_pool_ptr, write_handler, (void *)ret_cli);
          break;
        default:
          printf("epoll events error\n");
      }
    }
    printf("cycle done\n");
  }
}

int initSock(int epfd, client_t *ep_server_ptr) {
  int server_sockfd = Socket(PROTOCOL, SOCK_STREAM, 0);
  struct sockaddr_in server_addr;
  int opt = 1;  // 端口复用
  int flag;

  // initialization
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = PROTOCOL;
  server_addr.sin_addr.s_addr = htonl(SERVER_IP);
  server_addr.sin_port = htons(SERVER_PORT);

  // 非阻塞IO
  flag = fcntl(server_sockfd, F_GETFL);
  flag |= O_NONBLOCK;
  if (fcntl(server_sockfd, F_SETFL, flag) == -1) {
    err_sys("server_sockfd, fcntl error");
    exit(1);
  }
  // 设置端口复用
  setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  // Bind
  Bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  Listen(server_sockfd, LISTEN_BACKLOG);

  // return event
  client_set(ep_server_ptr, server_sockfd, EPOLLIN);
  // add to epoll
  client_add(epfd, EPOLLIN, ep_server_ptr);

  return server_sockfd;
}

int client_set(client_t *ep_server_ptr, int sockfd, int event) {
  bzero(ep_server_ptr, sizeof(*ep_server_ptr));
  ep_server_ptr->sockfd = sockfd;
  ep_server_ptr->event = event;
  ep_server_ptr->status = CLIENT_WEPOLL;
  bzero(ep_server_ptr->msg, BUFFERSIZE);
  ep_server_ptr->msg_len = 0;
  ep_server_ptr->last_act = time(NULL);
  return 0;
}

int client_add(int epfd, int event, client_t *ptr) {
  struct epoll_event ep_event;
  bzero(&ep_event, sizeof(ep_event));
  ep_event.events = event | EPOLLET;
  ep_event.data.ptr = (void *)ptr;
  Epoll_ctl(epfd, EPOLL_CTL_ADD, ptr->sockfd, &ep_event);
}

int client_del(int epfd, client_t *ptr) {
  if (ptr->status == CLIENT_OFFLINE) {
    return -1;
  }
  ptr->status = CLIENT_OFFLINE;
  Epoll_ctl(epfd, EPOLL_CTL_DEL, ptr->sockfd, NULL);
  return 0;
}

int client_mod(int epfd, int event, client_t *ptr, char *msg, size_t sz) {
  struct epoll_event ep_event;
  bzero(&ep_event, sizeof(ep_event));
  ptr->event = event;
  ptr->status = CLIENT_THR_ACT;
  strncpy(ptr->msg, msg, sz);
  ptr->msg_len = sz;
  ptr->last_act = time(NULL);
  ep_event.events = event | EPOLLET;
  ep_event.data.ptr = (void *)ptr;
  Epoll_ctl(epfd, EPOLL_CTL_MOD, ptr->sockfd, &ep_event);
}

void connection_handler(int epfd, client_t *ret_cli) {
  struct sockaddr_in client_addr;
  int client_sockfd, client_addr_len;
  char cli_ip[BUFFERSIZE];
  int i, flag, opt = 1;

  bzero(&client_addr, sizeof(client_addr));
  client_sockfd = Accept(ret_cli->sockfd, (struct sockaddr *)&client_addr,
                         &client_addr_len);
  printf("received connection ... \nip\t:%s\nport\t:%d\n",
         inet_ntop(PROTOCOL, &(client_addr.sin_addr.s_addr), cli_ip,
                   sizeof(cli_ip)),
         ntohs(client_addr.sin_port));
  do {
    for (i = 0; i < EPOLL_CLIENT_MAX; ++i) {
      if (clients[i].status == -1) {
        break;
      }
    }
    if (i == EPOLL_CLIENT_MAX) {
      // TODO: 使用条件变量，进入等待
      printf("too many connection ... closing ...\n");
      break;
    }
    // 非阻塞IO
    flag = fcntl(client_sockfd, F_GETFL);
    flag |= O_NONBLOCK;
    if (fcntl(client_sockfd, F_SETFL, flag) == -1) {
      err_sys("client_sockfd, fcntl error");
      break;
    }
    client_set(&(clients[i]), client_sockfd, EPOLLIN);
    client_add(epfd, EPOLLIN, &(clients[i]));
    return;
  } while (0);

  shutdown(client_sockfd, SHUT_RDWR);
}

void read_handler(void *arg) {
  client_t *cli_ptr = (client_t *)arg;
  char buf[BUFFERSIZE], *buf_ptr;
  ssize_t sz;
  int nleft = BUFFERSIZE;

  buf_ptr = buf;
  do {
  _read_again:
    while (nleft > 0 &&
           ((sz = read(cli_ptr->sockfd, buf_ptr, BUFFERSIZE)) > 0)) {
      buf_ptr += sz;
      nleft -= sz;
    }
    if (sz < 0 && errno != EAGAIN) {
      if (errno == EINTR) {
        goto _read_again;
      }
      printf("read error");
      break;
    } else if (sz == 0) {
      break;
    }
    printf("read:%s\n", buf);
    client_mod(epfd, EPOLLOUT, cli_ptr, buf, buf_ptr - buf);
    printf("socket read end\n");
    return;
  } while (0);
  // close
  shutdown(cli_ptr->sockfd, SHUT_RDWR);
  printf("socket closed\n");
}

void write_handler(void *arg) {
  client_t *cli_ptr = (client_t *)arg;
  size_t sz;
  do {
  _write_again:
    if ((sz = write(cli_ptr->sockfd, cli_ptr->msg, cli_ptr->msg_len)) < 0) {
      if (errno == EINTR) {
        goto _write_again;
      }
      break;
    } else if (sz == 0) {
      printf("client connection closed\n");
      break;
    } else {  // sz > 0
      printf("write done\n");
      client_mod(epfd, EPOLLIN, cli_ptr, NULL, 0);
      return;
    }
  } while (0);
  shutdown(cli_ptr->sockfd, SHUT_RDWR);
}