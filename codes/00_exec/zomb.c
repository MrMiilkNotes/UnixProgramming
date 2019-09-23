#include <sys/wait.h>
#include <time.h>
#include "../include/apue.h"

int main(void) {
  pid_t pid;
  int count;

  for (count = 0; count < 5; ++count) {
    if ((pid = fork()) < 0) {
      printf("fork error: %dth\n", count);
    } else if (pid == 0) {
      printf("in sub process %dth\n", count);
      sleep(1);
      break;
    }
  }
  if (count == 5) {
    printf("in parent\n");
    sleep(100);
  }
  printf("end, c = %d\n", count);
  exit(0);
}