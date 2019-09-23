#include"../include/apue.h"
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>

#define PROTOCOL AF_INET
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFERSIZE BUFSIZ

int main(void) {
    struct sockaddr_in server_addr;
    int sock_server;
    char msg[BUFFERSIZE];
    char buf[BUFFERSIZE];
    int sz_write, sz_read;

    if((sock_server = socket(PROTOCOL, SOCK_STREAM, 0)) == -1) {
        err_sys("socket error");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = PROTOCOL;
    server_addr.sin_port = htons(SERVER_PORT);
    if(inet_pton(PROTOCOL, SERVER_IP, &(server_addr.sin_addr.s_addr)) != 1) {
        err_sys("inet_pton error");
        exit(1);
    }

    if (connect(sock_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        err_sys("connect error");
        exit(1);
    }

    while(1) {
        printf(">>> ");
        fgets(msg, sizeof(msg), stdin);
        if((sz_write = write(sock_server, msg, strlen(msg))) < 0) {
            err_sys("write error");
        } else if (sz_write != strlen(msg)) {
            printf("write error: write %d cahracter, expect: %lu\n", 
                    sz_write, strlen(msg));
        } else if ((sz_read = read(sock_server, buf, sizeof(buf))) < 0) {
            err_sys("read error");
        } else {
            printf("received: '%s'\n", buf);
        }
    }

    printf("close ...\n");
    close(sock_server);
    return 0;
}