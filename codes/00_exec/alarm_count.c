#include <signal.h>
#include <time.h>
#include "../include/apue.h"

static void stop(int signo) { printf("catch %d\n", signo); }

int main(void) {
  int i;
  alarm(1);

  signal(SIGALRM, stop);

  sleep(3);

  exit(0);
}