#include "../include/apue.h"
#include "./mmap_anno.h"

int main(void) {
  int status;
  int fd;
  const char *zero_pathname = "./tmp";
  MSG *msg_body;
  msg_body = (MSG *)malloc(sizeof(MSG));

  if ((fd = open(zero_pathname, O_RDONLY)) < 0) {
    err_sys("open error");
    exit(1);
  }

  if ((msg_body = (MSG *)mmap(NULL, sizeof(MSG), PROT_READ, MAP_SHARED, fd,
                              0)) == MAP_FAILED) {
    err_sys("mmap failed");
    exit(1);
  }

  while (1) {
    printf("receive msg: \n");
    printf("id: %d\n", msg_body->id);
    printf("name: %s", msg_body->name);
    sleep(1);
  }

  if (munmap(msg_body, sizeof(MSG)) < 0) {
    err_sys("munmap failed");
    exit(0);
  }
  close(fd);

  exit(0);
}