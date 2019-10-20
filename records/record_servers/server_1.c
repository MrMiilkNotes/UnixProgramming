/*
 * socket通讯，只能进行一次通讯
 * 单进程处理,一次性应答
 */

#include <arpa/inet.h>
#include <ctype.h>   // toupper
#include <string.h>  // memset
#include <sys/socket.h>
#include <sys/types.h>
#include "../include/apue.h"

#define PROTOCOL AF_INET  // IPv4
#define PORT 8080         // 端口
#define IP "127.0.0.1"    // 指定IP地址
#define BACKLOG 3         // listen设置的最大等待连接数量
#define BUFFERSIZE BUFSIZ
/*
 * socket连接的建立：
 *  使用soocket函数“创建”socket文件
 *  用bind绑定socket文件和具体的IP+port -> 缓冲区
 *  用listen指定最大的未处理连接 pending connections
 *  使用accept接受连接
 */

int main(void) {
  struct sockaddr_in server_addr, cli_addr;
  /*
  * 主要记录协议以及IP，port
  *     bind的时候需要提供server的相关信息
  *     从accept返回的时候携带client的相关信息
  */
  int sockfd_s, sockfd_c;   // socket文件描述符
  int cli_addr_len;         //accept的传出参数

  // 用于伪装server处理request的参数
  int i, sz;
  char buf[BUFFERSIZE], cli_ip[BUFFERSIZE];

  /* -----------------------------------------------------------------------------------
  * socket 初始化
  *     包括到listen的步骤
  */
  if ((sockfd_s = socket(PROTOCOL, SOCK_STREAM, 0)) == -1) {
    err_sys("socket error");
    exit(1);
  }

  // 可以使用bzero函数。填写server的ip+port
  memset(&cli_addr, 0, sizeof(cli_addr));
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = PROTOCOL;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // INADDR_ANY 使用可用的任意IP
  // 如果用点分十进制字符串指定ip可以使用inet_pton
  // if(inet_pton(PROTOCOL, IP, &(server_addr.sin_addr.s_addr)) != 1) {
  //     err_sys("inet_pton error");
  //     exit(1);
  // }

  if (bind(sockfd_s, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    err_sys("bind error");
    exit(1);
  }

  if (listen(sockfd_s, BACKLOG) == -1) {
    err_sys("listen error");
    exit(1);
  }

  /* -----------------------------------------------------------------------------------
  * 接受连接请求并处理
  *     这里accept会将client的ip+port放在sockaddr结构中
  *     最后一个是传入传出参数
  *     返回client的socket文件描述符，特别地回想一下一个socket文件描述符对应两个缓冲区
  */
  cli_addr_len = sizeof(cli_addr);
  if ((sockfd_c = accept(sockfd_s, (struct sockaddr *)&cli_addr,
                         &cli_addr_len)) == -1) {
    err_sys("accept error");
    exit(1);
  }

  printf(
      "received connection ... \nip\t:%s\nport\t:%d\n",
      inet_ntop(AF_INET, &(cli_addr.sin_addr.s_addr), cli_ip, sizeof(cli_ip)),
      ntohs(cli_addr.sin_port));

  // 简单地读取和返回
  if ((sz = read(sockfd_c, buf, BUFFERSIZE)) == -1) {
    err_sys("read error");
    // close
  } else if (sz != 0) {
    printf("received: %s\n", buf);
    for (i = 0; i < sz; ++i) {
      buf[i] = toupper(buf[i]);
    }
    if (write(sockfd_c, buf, sz) != sz) {
      err_sys("write error");
      // close
    }
  }

  printf("close ...\n");
  close(sockfd_c);
  close(sockfd_s);
  return 0;
}