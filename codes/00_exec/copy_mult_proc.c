#include "../include/apue.h"
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define BUFFSIZE 1024

// copy_mult_proc <from> <to> <num_of_proc>
int main(int argc, char *argv[]) {
    pid_t pid;
    int i, num_proc;
    int i_fd, o_fd;
    char buf[BUFFSIZE];
    char *o_map_ptr, *i_map_ptr;
    struct stat stat_buf;
    ssize_t sz, f_sz;
    off_t pad;
    int len;
    int use_sig, child_stat;
    sigset_t sigset, old_sigset;

    // check arguments
    if(argc == 4) {
        num_proc = atoi(argv[3]);
    } else if(argc == 3){
        num_proc = 10;
    } else {
        printf("Usage: copy_mult_proc <from> <to> <num_of_proc>");
        exit(1);
    }

    /*
     *  打开文两个文件
     *  获取读文件长度，设置写文件长度
     *  将两个全部映射到内存 
     */
    if ((i_fd = open(argv[1], O_RDONLY)) < 0) {
        err_sys("open input file error");
        exit(1);
    }
    if (stat(argv[1], &stat_buf) < 0) {
        err_sys("stat error");
        exit(1);
    } else {
        f_sz = stat_buf.st_size;
    }
    if ((o_fd = open(argv[2], O_RDWR | O_CREAT)) < 0) {
        err_sys("open output file error");
        exit(1);
    }
    ftruncate(o_fd, f_sz);
    if ((i_map_ptr = mmap(NULL, f_sz, PROT_READ, MAP_PRIVATE, i_fd, 0)) == MAP_FAILED) {
        err_sys("input file mmap failed");
    }
    if ((o_map_ptr = mmap(NULL, f_sz, PROT_READ | PROT_WRITE, MAP_SHARED, o_fd, 0)) == MAP_FAILED) {
        err_sys("output file mmap failed");
    }

    /*
     *  创建多进程 
     */
    for(i = 0; i < num_proc; ++i) {
        if((pid = fork()) < 0) {
            printf("creating process: %d\n", i);
            err_sys("create process error");
            exit(1);
        } else if(pid == 0) {
            break;
        }
    }

    /* 
     *  父进程等待子进程处理
     */
    if (pid > 0) {
        // parent
        do {
            if (waitpid(0, &child_stat, 0) > 0) {
                --num_proc;
            } 
            // else {
            //     err_sys("waitpid error");
            //     exit(1);
            // }
        } while (num_proc > 0);
        printf("wait finished\n");
    }

    /*
     *  子进程执行复制 
     */
    if(pid == 0) {
        // copy
        pad = (f_sz / num_proc + 1) * i;
        if (i == num_proc - 1) {
            len = f_sz - pad;
        } else {
            len = f_sz / num_proc + 1;
        }
        printf("task: copy, pad = %ld, len = %d\n", pad, len);
        o_map_ptr += pad;
        i_map_ptr += pad;
        strncpy(o_map_ptr, i_map_ptr, len);
        printf("copy: %d\n", len);
        exit(0);
    }

    // end
    munmap(i_map_ptr, f_sz);
    munmap(o_map_ptr, f_sz);
    exit(0);
}