/*
 * socket通讯，多进程版本
 *  - 第一个版本的最主要问题在于只能处理单个，单次的请求，server会直接退出
 *      因而改进的方式就是利用并发，可以使用多进程，多线程等方式
 *  - 这里使用的是多进程的方式，因此注册SIGCHLD信号处理函数来避免僵尸进程
 *  - 由于对几个函数都要进行出错检查，因此将socket的几个函数简单封装
 */
#include <arpa/inet.h>
#include <ctype.h>
#include <signal.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../include/apue.h"
#include "readn.h"
#include "sock_func.h"

#define SERVER_IP "127.0.0.1"  // or try INADDR_ANY
#define SERVER_PORT 8080
#define PROTOCOL AF_INET
#define LISTEN_BACKLOG 128
#define BUFFERSIZE BUFSIZ

void pr_exit(int status);         // 用于打印子进程的结束状态
void sigchld_handler(int signo);  // 子进程结束的处理函数

int main(void) {
  // socket
  struct sockaddr_in addr_server, addr_client;
  int sockfd_server, sockfd_client;
  int addr_client_len;
  // subprocess
  pid_t pid;
  char buf[BUFFERSIZE], cli_ip[BUFFERSIZE];
  int i;
  ssize_t sz;
  // signal
  struct sigaction sigchld_act;
  sigset_t sigchld_mask;

  // 这部分和server_1是一样的，可以看到变得更简短了
  sockfd_server = Socket(PROTOCOL, SOCK_STREAM, 0);
  addr_server.sin_family = PROTOCOL;
  addr_server.sin_port = htons(SERVER_PORT);
  inet_pton(PROTOCOL, SERVER_IP, &(addr_server.sin_addr.s_addr));
  Bind(sockfd_server, (struct sockaddr *)&addr_server, sizeof(addr_server));
  Listen(sockfd_server, LISTEN_BACKLOG);

  // SIGCHLD处理函数注册
  sigemptyset(&sigchld_mask);
  sigchld_act.sa_handler = sigchld_handler;
  sigchld_act.sa_mask = sigchld_mask;
  sigchld_act.sa_flags = 0;
  sigchld_act.sa_flags |= SA_RESTART;  // 指定可以重启动，读写socket可能遇到中断
  if (sigaction(SIGCHLD, &sigchld_act, NULL) == -1) {
    err_sys("sigaction error");
    exit(1);
  }

  bzero(&addr_client, sizeof(addr_client));
  // -----------------------------------------------------------------------------
  // 不断接受连接，新连接会交由子进程处理
  while (sockfd_client = Accept(sockfd_server, (struct sockaddr *)&addr_client,
                                &addr_client_len)) {
    if ((pid = fork()) < 0) {
      err_sys("fork error");
      // TODO: error handle process
      close(sockfd_client);
    } else if (pid > 0) {
      // ==========================
      //    >>>> parent <<<<
      // ==========================
      printf("received connection ... \nip\t:%s\nport\t:%d\n",
             inet_ntop(AF_INET, &(addr_client.sin_addr.s_addr), cli_ip,
                       sizeof(cli_ip)),
             ntohs(addr_client.sin_port));
      bzero(&addr_client, addr_client_len);
      // 父进程关闭client的socket文件描述符(!!!!!!)
      close(sockfd_client);
    }
    // ==========================
    //      >>>> child <<<<
    // ==========================
    if (pid == 0) {
      close(sockfd_server);  // (!!)
      // 读取一行
      while ((sz = sock_readline(sockfd_client, buf, BUFFERSIZE)) != 0) {
        if (sz == -1) {
          printf("sock_readline error\n");
          exit(1);
        }
        printf("read from socket: %s", buf);
        for (i = 0; i < sz; ++i) {
          buf[i] = toupper(buf[i]);
        }
        if ((sz = write(sockfd_client, buf, sz)) != sz) {
          err_sys("write error");
        }
      }
      printf("closing a socket ...\n");
      close(sockfd_client);
      exit(0);
    }
  }
  return 0;
}

void pr_exit(int status) {
  // 这个函数的编写来自APUE
  if (WIFEXITED(status)) {
    printf("normal termination, exit status: %d\n", WEXITSTATUS(status));
  } else if (WIFSIGNALED(status)) {
    printf("abnormal termation, signal number = %d\n", WTERMSIG(status));
  }
}

void sigchld_handler(int signo) {
  pid_t pid;
  int status;
  /*
   *   注意：这里使用while而非if，可以看markdown笔记
   *       当信号已经进入处理的时候，可能会有多个信号发生，由于是不可靠信号
   *       因此可能出现信号发生多次而只进入处理函数处理一次
   *       好在可以使用waitpid循环处理，从而避免僵尸进程
   */
  while ((pid = waitpid(0, &status, WNOHANG)) != 0) {
    pr_exit(status);
  }
}