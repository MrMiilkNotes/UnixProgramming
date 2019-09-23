#include"../include/apue.h"
#include <sys/stat.h>
#include<fcntl.h>
#include<sys/wait.h>

int main(void) {
    pid_t pid;
    int fd;
    const char* filename = "tmp.1";
    int i, NUM_PROC = 5;
    int status;

    for(i = 0; i < NUM_PROC; ++i) {
        if((pid = fork()) < 0) {
            err_sys("fork error");
            exit(1);
        } else if (pid == 0) {
            // child
            break;
        }
    }

    if(pid == 0) {
        printf("process %d, opening file ...\n", i + 1);
        if ((fd = open(filename, O_RDWR | O_CREAT)) == -1) {
            err_sys("open file error");
            exit(1);
        }
        printf("seeking ...\n");
        lseek(fd, i*100, SEEK_SET);
        write(fd, "Hello world\n", sizeof("Hello world\n"));
        sleep(2);
        printf("process %d finishing ...\n", i + 1);
        exit(0);
    }

    if(pid > 0) {
        int count = NUM_PROC;
        do {
            if((i = waitpid(0, &status, 0)) > 0) {
                --count;
            } else if(i == -1) {
                err_sys("waitpid error");
                exit(1);
            }
        } while(count > 0);
        printf("test finished\n");
    }
}