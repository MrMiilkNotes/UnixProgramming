#include "../include/apue.h"
#include "./mmap_anno.h"

int main(void) {
  int status;
  int fd;
  const char *zero_pathname = "./tmp";
  MSG *msg_body, *_msg;
  _msg = (MSG *)malloc(sizeof(MSG));
  _msg->id = 10;
  strcpy(_msg->name, "MrMiilk");

  if ((fd = open(zero_pathname, O_RDWR | O_CREAT), 0644) < 0) {
    err_sys("open error");
    exit(1);
  }

  if ((msg_body = (MSG *)mmap(NULL, sizeof(MSG), PROT_READ | PROT_WRITE,
                              MAP_SHARED, fd, 0)) == MAP_FAILED) {
    err_sys("mmap failed");
    exit(1);
  }

  while (1) {
    sleep(1);
    memcpy(msg_body, _msg, sizeof(_msg));
    _msg->id += 1;
    printf("msg send...\n");
    printf("id: %d\n", _msg->id);
    printf("name: %s\n", _msg->name);
  }

  if (munmap(msg_body, sizeof(MSG)) < 0) {
    err_sys("munmap failed");
    exit(0);
  }
  close(fd);

  exit(0);
}