#include<time.h>
#include<string.h>
#include<strings.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<arpa/inet.h>
#include"../include/apue.h"
#include"epoll_func.h"
#include"sock_func.h"

#define PROTOCOL AF_INET
#define SERVER_IP INADDR_ANY
#define SERVER_PORT 8080
#define LISTEN_BACKLOG 128
#define EPOLL_INITSIZE 10
#define EPOLL_RET_SIZE EPOLL_INITSIZE
#define BUFFERSIZE BUFSIZ

typedef struct _event_data {
    int status;                     // 记录节点状态，-1：未使用，0：经过初始化，1：已经记录在epfd
    int event;                      // 记录对应的监听类型
    int sockfd;                     // 监听的sock
    void *arg;                      // 指向本身的指针，用于传入handler
    void (*handler) (int, void *);  // 事件满足的处理函数
    char buf[BUFFERSIZE];           // 记录返回给client的数据
    size_t len;                     // 数据长度
    time_t last_active;             // client上次活跃时间，用于清除长时间连接的不活跃用户
} event_data_t;

// ---------------------------------------
// global variable
// ---------------------------------------
int epfd;
event_data_t clients[EPOLL_INITSIZE + 1];

// ---------------------------------------
// event handlers
// ---------------------------------------
void conn_handler(int server_sockfd, void *arg);
void read_handler(int sockfd, void *arg);
void write_handler(int sockfd, void *arg);
// ---------------------------------------
// functions for struct enent_data_t
// ---------------------------------------
void event_set(int sockfd, void(*handler)(int, void *), event_data_t* ev);    // 设置回调函数
void event_add(int epfd, int event, event_data_t *ev);    // 添加到epoll监听
void event_mod(int epfd, int event, void(*handler)(int, void *), event_data_t* ev);
void event_del(int epfd, event_data_t *ev);    // 从epoll监听中删除

int initSocket(int epfd, event_data_t* ev);

int main(void) {
    epfd = Epoll_create(EPOLL_INITSIZE);
    int i, nready;
    struct epoll_event ret_events[EPOLL_INITSIZE + 1];
    event_data_t *ret_event;

    // ---------------------------------------
    // initialization
    // ---------------------------------------
    // 用 status=-1 表示未使用
    for(i = 0; i <= EPOLL_INITSIZE; ++i) {
        clients[i].status = -1;
    }

    // ---------------------------------------
    // 设置socket
    // ---------------------------------------
    initSocket(epfd, &clients[EPOLL_INITSIZE]);

    while(1) {
        nready = Epoll_wait(epfd, ret_events, EPOLL_INITSIZE + 1, -1);
        if (nready == -1) {
            err_sys("epoll_wait error");
            exit(1);
        }
        for(i = 0; i < nready; ++i) {
            ret_event = ret_events[i].data.ptr;
            ret_event->handler(ret_event->sockfd, ret_event->arg);
        }
    }

    return 0;
}

void event_set(int sockfd, void(*handler)(int, void *), event_data_t* ev) {
    ev->status = 0;
    ev->sockfd = sockfd;
    ev->arg = ev;   // 记录本身
    ev->handler = handler;
    bzero(ev->buf, BUFFERSIZE);
    ev->len = 0;
    ev->last_active = time(NULL);
}

void event_add(int epfd, int event, event_data_t *ev) {
    int op;
    struct epoll_event ep_event;

    if(ev->status == 0) {
        op = EPOLL_CTL_ADD;
        ev->status = 1;
    } else {
        op = EPOLL_CTL_MOD;
    }
    ep_event.events = ev->event = event;
    ep_event.data.ptr = ev;
    Epoll_ctl(epfd, op, ev->sockfd, &ep_event);
}

void event_mod(int epfd, int event, void(*handler)(int, void *), event_data_t* ev) {
    struct epoll_event ep_event;
    ev->handler = handler;
    ev->last_active = time(NULL);
    ep_event.events = ev->event = event;
    ep_event.data.ptr = ev;
    Epoll_ctl(epfd, EPOLL_CTL_MOD, ev->sockfd, &ep_event);
}

void event_del(int epfd, event_data_t *ev) {
    if(ev->status == -1) return;
    Epoll_ctl(epfd, EPOLL_CTL_DEL, ev->sockfd, NULL);
    ev->status = -1;
}

int initSocket(int epfd, event_data_t* ev) {
    int opt = 1;    // 端口复用
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
    server_sockfd = Socket(PROTOCOL, SOCK_STREAM, 0);   // TCP
    // 设置非阻塞IO
    flag = fcntl(server_sockfd, F_GETFL);
    flag |= O_NONBLOCK;
    if(fcntl(server_sockfd, F_SETFL, flag) == -1) {
        err_sys("server_sockfd, fcntl error");
        exit(1);
    }
    // 设置端口复用
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // 设置socket
    Bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
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
    int flag;   // 非阻塞IO

    client_sockfd = Accept(server_sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
    event_data_ptr->last_active = time(NULL);
    printf("received connection ... \nip\t:%s\nport\t:%d\n", \
            inet_ntop(PROTOCOL, &(client_addr.sin_addr.s_addr), cli_ip, sizeof(cli_ip)), \
            ntohs(client_addr.sin_port)
    );
    
    for(i = 0; i < EPOLL_INITSIZE; ++i) {
        if(clients[i].status == -1) {
            ev_ptr = clients + i;
            break;
        }
    }
    do {
        if(i == EPOLL_INITSIZE) {
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
        // else
        event_set(client_sockfd, read_handler, ev_ptr);
        event_add(epfd, EPOLLIN, ev_ptr);
    } while(0);
}

void read_handler(int sockfd, void *arg) {
    event_data_t *event_data_ptr = (event_data_t *)arg;
    char buf[BUFFERSIZE];
    ssize_t sz;
_read_again:
    if((sz = read(sockfd, buf, BUFFERSIZE)) <= 0) {
        if (sz < 0) {
            if(errno == EAGAIN || errno == EINTR) {
                goto _read_again;
            }
            err_sys("read from socket error");
        } else {
            printf("socket closed by client");
        }
        shutdown(sockfd, SHUT_RD);
        event_del(epfd, event_data_ptr);
    } else {
        // int epfd, int event, void(*handler)(int, void *), event_data_t* ev
        strncpy(event_data_ptr->buf, buf, sz);
        event_data_ptr->len = sz;
        event_mod(epfd, EPOLLOUT, write_handler, event_data_ptr);
    }
}

void write_handler(int sockfd, void *arg) {
    event_data_t *event_data_ptr = (event_data_t *)arg;
    ssize_t sz;
_write_again:
    if((sz = write(sockfd, event_data_ptr->buf, event_data_ptr->len)) != event_data_ptr->len) {
        if(sz == -1 && (errno == EAGAIN || errno == EINTR)) {
            goto _write_again;
        }
        err_sys("socket write error");
        shutdown(sockfd, SHUT_RD);
        event_del(epfd, event_data_ptr);
    } else {
        event_mod(epfd, EPOLLIN, read_handler, event_data_ptr);
    }
}