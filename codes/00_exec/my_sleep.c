#include <signal.h>
#include "../include/apue.h"

int main(void) {}

void alarm_handler(int signo) { return; }

unsigned my_sleep(int sec) {
  struct sigaction act, act_old;
  act.sa_handler = alarm_handler;
  sigemptyset(act.sa_mask);
  act.sa_flag = 0;
  if (sigaction(SIGALRM, &act, &act_old) < 0) {
    // err
    exit(1);
  }
  alarm(sec);
  if (pause() != -1) {
    // err
    exit(1);
  }

  // recover, errors should recover too.
  sigaction(SIGALRM, &act_old, NULL);
  return alarm(0);
}