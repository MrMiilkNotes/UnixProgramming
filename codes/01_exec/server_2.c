#include"../include/apue.h"
#include"sock_func.h"
#include<ctype.h>
#include<strings.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/wait.h>

#define SERVER_IP "127.0.0.1"    //or try INADDR_ANY
#define SERVER_PORT 8080
#define PROTOCOL AF_INET
#define LISTEN_BACKLOG 128
#define BUFFERSIZE BUFSIZ

ssize_t sock_readline(int sockfd, void *buf, size_t maxlen);

void pr_exit(int status) {
  if (WIFEXITED(status)) {
    printf("normal termination, exit status: %d\n", WEXITSTATUS(status));
  } else if (WIFSIGNALED(status)) {
    printf("abnormal termation, signal number = %d\n", WTERMSIG(status));
  }
}

void sigchld_handler(int signo) {
    pid_t pid;
    int status;
    while((pid = waitpid(0, &status, WNOHANG)) != 0) {
        pr_exit(status);
    }
}

int main(void) {
    // socket
    struct sockaddr_in addr_server, addr_client;
    int sockfd_server, sockfd_client;
    int addr_client_len;
    // subprocess
    pid_t pid;
    char buf[BUFFERSIZE];
    int i;
    ssize_t sz;
    // signal
    struct sigaction sigchld_act;
    sigset_t sigchld_mask;

    sockfd_server = Socket(PROTOCOL, SOCK_STREAM, 0);
    addr_server.sin_family = PROTOCOL;
    addr_server.sin_port = htons(SERVER_PORT);
    inet_pton(PROTOCOL, SERVER_IP, &(addr_server.sin_addr.s_addr));
    Bind(sockfd_server, (struct sockaddr *)&addr_server, sizeof(addr_server));
    Listen(sockfd_server, LISTEN_BACKLOG);

    // SIGCHLD
    sigemptyset(&sigchld_mask);
    sigchld_act.sa_handler = sigchld_handler;
    sigchld_act.sa_mask = sigchld_mask;
    sigchld_act.sa_flags = 0;
    if (sigaction(SIGCHLD, &sigchld_act, NULL) == -1) {
        err_sys("sigaction error");
        exit(1);
    }


    bzero(&addr_client, sizeof(addr_client));
    while(sockfd_client = Accept(sockfd_server, 
            (struct sockaddr *)&addr_client, &addr_client_len)) {
        if((pid = fork()) < 0) {
            err_sys("fork error");
            // TODO: error handle process
            close(sockfd_client);    
        } else if (pid == 0) {
            break;
        } else {
            // parent
            bzero(&addr_client, addr_client_len);
            close(sockfd_client);
        }

        // child
        if (pid == 0) {
            close(sockfd_server);
            while((sz = sock_readline(sockfd_client, buf, BUFFERSIZE)) != 0) {
                if (sz == -1) {
                    printf("sock_readline error\n");
                    exit(1);
                }
                printf("read from socket: %s", buf);
                for(i = 0; i < sz; ++i) {
                    buf[i] = toupper(buf[i]);
                }
                if((sz = write(sockfd_client, buf, sz)) != sz) {
                    err_sys("write error");
                }
                bzero(&addr_client, sizeof(addr_client));
            }
            printf("closing a socket ...\n");
            close(sockfd_client);
            exit(0);
        }
    }
    return 0;
}