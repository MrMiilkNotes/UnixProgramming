#include <signal.h>
#include "../include/apue.h"

void handler(int aigno) { printf("catch signal SIGQUIT\n"); }

int main(void) {
  struct sigaction sact;

  sact.sa_handler = handler;
  sigemptyset(&sact.sa_mask);
  //   sigaddset(&sact.sa_mask, SIGSTOP);
  //   sigaddset(&sact.sa_mask, SIGQUIT);
  sact.sa_flags = 0;

  if (sigaction(SIGQUIT, &sact, NULL) < 0) {
    printf("sigaction failed\n");
    exit(1);
  }
  printf("sigaction setted\n");

  while (1)
    ;

  exit(0);
}