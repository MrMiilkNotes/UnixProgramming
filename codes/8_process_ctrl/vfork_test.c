#include "../include/apue.h"

int glob_var = 6;

int main(int argc, char const *argv[]) {
  int var;
  pid_t pid;

  var = 8;

  printf("before fork\n");
  /* donot flush stdout */

  if ((pid = vfork()) < 0) {
    err_sys("vfork error");
  } else if (pid == 0) {
    // child
    ++glob_var;
    ++var;
    _exit(0);   // end of child
  } else {
    // parent
    sleep(2);
  }

  printf("pid = %ld, glob = %d, var = %d\n", (long)getpid(), glob_var, var);
  exit(0);
}
