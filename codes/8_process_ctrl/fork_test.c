#include "../include/apue.h"

int glob_var = 6;
char buf[] = "a write to stdout\n";

int main(int argc, char const *argv[]) {
  int var;
  pid_t pid;

  var = 8;
  if (write(STDOUT_FILENO, buf, sizeof(buf) - 1) != sizeof(buf) - 1)
    err_sys("write error");

  printf("before fork\n");
  /* donot flush stdout */

  if ((pid = fork()) < 0) {
    err_sys("fork error");
  } else if (pid == 0) {
    // child
    ++glob_var;
    ++var;
  } else {
    // parent
    sleep(2);
  }

  printf("pid = %ld, glob = %d, var = %d\n", (long)getpid(), glob_var, var);
  return 0;
}
