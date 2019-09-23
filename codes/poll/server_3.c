/* 
 * 多路IO转接模型
 * 使用 select
 */
 #include<unistd.h>
 #include<sys/time.h>
 #include<sys/types.h>
 #include<sys/socket.h>
 #include<arpa/inet.h>
 #include<strings.h>
 #include"../include/apue.h"
 #include"sock_func.h"
 #include"readn.h"

#define SERVER_IP INADDR_ANY
#define SERVER_PORT 8080
#define PROTOCOL AF_INET
#define BUFFERSIZE BUFSIZ
#define LISTEN_BACKLOG 128

int main(void) {
    // socket
    struct sockaddr_in server_addr, client_addr;
    int server_sockfd, client_sockfd;
    int i, fdset[FD_SETSIZE], client_addr_sz;       // i用于迭代
                                                    // fdset 记录client的sockfd
                                                    // client_addr_sz 为accept参数
    ssize_t sz;
    char cli_ip[BUFFERSIZE], msg[BUFFERSIZE];
    // select
    fd_set read_set, readable_set;
    int nready, maxfds;         // maxfds 记录最大的描述符值，用于select
                                // nready 用于记录select返回值，有多少个连接准备好  
    // initialization
    for(i = 0; i < FD_SETSIZE; ++i) {
        fdset[i] = -1;
    }
    FD_ZERO(&read_set);
    maxfds = -1;

    server_sockfd = Socket(PROTOCOL, SOCK_STREAM, 0);
    maxfds = server_sockfd;
    // TODO: 端口复用

    // bind addrrss
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = PROTOCOL;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(SERVER_IP);
    Bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    Listen(server_sockfd, LISTEN_BACKLOG);

    // server_sockfd 不作为客户对象
    FD_SET(server_sockfd, &read_set);
    while(1) {
        // printf("new select ...\n");
        readable_set = read_set;
        nready = select(maxfds + 1, &readable_set, NULL, NULL, NULL);

        if(FD_ISSET(server_sockfd, &readable_set)) {
            --nready;
            // 新的连接请求
            bzero(&client_addr, sizeof(client_addr));
            client_sockfd = Accept(server_sockfd, (struct sockaddr *)&client_addr, &client_addr_sz);
            printf("received connection ... \nip\t:%s\nport\t:%d\n", \
                    inet_ntop(AF_INET, &(client_addr.sin_addr.s_addr), cli_ip, sizeof(cli_ip)), \
                    ntohs(client_addr.sin_port)
            );
            // record new client
            for(i = 0; i < FD_SETSIZE; ++i) {
                if(fdset[i] == -1) {
                    fdset[i] = client_sockfd;
                    maxfds = maxfds > client_sockfd ? maxfds : client_sockfd;
                    FD_SET(client_sockfd, &read_set);
                    break;
                }
            }
            if (i == FD_SETSIZE) {
                printf("too many socket conection, closing client: %s\n", cli_ip);
                close(client_sockfd);
            }
        }
        // 有客户已经有数据可读
        for(i = 0; i < FD_SETSIZE; ++i) {
            if( nready <= 0) break; 
            if(fdset[i]!= -1 && FD_ISSET(fdset[i], &readable_set)) { 
                nready--;
                client_sockfd = fdset[i];
                if((sz = sock_readline(client_sockfd, msg, BUFFERSIZE)) != 0) {
                    if (sz < 0) {
                        err_sys("read error");
                    } else {
                        printf("received: '%s'\n", msg);
                        if((sz = write(client_sockfd, msg, sz)) != sz) {
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
    // TODO: close or shutdown
    close(server_sockfd);
    return 0;
}