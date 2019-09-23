/* 
 * socket通讯，只能进行一次通讯
 * 单进程处理,一次性应答
 */

#include"../include/apue.h"
#include<string.h>  // memset
#include<ctype.h>   // toupper
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>

#define PROTOCOL AF_INET
#define PORT 8080
#define IP "127.0.0.1"
#define BACKLOG 10
#define BUFFERSIZE BUFSIZ

int main(void) {
    struct sockaddr_in server_addr, cli_addr;
    int sockfd_s, sockfd_c;
    int cli_addr_len;

    int i, sz;
    char buf[BUFFERSIZE], cli_ip[BUFFERSIZE];

    if ((sockfd_s = socket(PROTOCOL, SOCK_STREAM, 0)) == -1) {
        err_sys("socket error");
        exit(1);
    }

    memset(&cli_addr, 0, sizeof(cli_addr));
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = PROTOCOL;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // if(inet_pton(PROTOCOL, IP, &(server_addr.sin_addr.s_addr)) != 1) {
    //     err_sys("inet_pton error");
    //     exit(1);
    // }

    if (bind(sockfd_s, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        err_sys("bind error");
        exit(1);
    }

    if (listen(sockfd_s, BACKLOG) == -1) {
        err_sys("listen error");
        exit(1);
    }

    cli_addr_len = sizeof(cli_addr);
    if((sockfd_c = accept(sockfd_s, (struct sockaddr *)&cli_addr, &cli_addr_len)) == -1) {
        err_sys("accept error");
        exit(1);
    }

    printf("received connection ... \nip\t:%s\nport\t:%d\n", \
            inet_ntop(AF_INET, &(cli_addr.sin_addr.s_addr), cli_ip, sizeof(cli_ip)), \
            ntohs(cli_addr.sin_port)
    );

    // handle request
    if ((sz = read(sockfd_c, buf, BUFFERSIZE)) == -1) {
        err_sys("read error");
        // close
    } else if(sz != 0) {
        printf("received: %s\n", buf);
        for(i = 0; i < sz; ++i) {
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