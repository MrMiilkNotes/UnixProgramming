#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "../include/apue.h"

int main(void) {
  int fd;
  int len = 10;  // file length
  char *shared_mem_ptr;

  if ((fd = open("tmp.out", O_CREAT | O_RDWR, 0644)) < 0) {
    err_sys("open file error");
    exit(1);
  }
  if (ftruncate(fd, len) != 0) {
    err_sys("ftruncate error");
    exit(1);
  }

  if ((shared_mem_ptr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                             0)) == MAP_FAILED) {
    err_sys("mmap failed");
    exit(1);
  }
  strcpy(shared_mem_ptr, "mann");

  if (munmap(shared_mem_ptr, len) == -1) {
    err_sys("munmap error");
    exit(1);
  }
  close(fd);
  //
  sleep(15);
  printf("wake up...\n");
  char buf[10];
  // strcpy(buf, shared_mem_ptr);
  printf("buf: %s\n", shared_mem_ptr);

  exit(0);
}