#include <sys/wait.h>
#include "../include/apue.h"

void pr_exit(int status) {
  /* 打印exit状态说明 */
  if (WIFEXITED(status)) {
    printf("normal termation, exit status = %d\n", WEXITSTATUS(status));
  } else if (WIFSIGNALED(status)) {
    printf("abnormal termation, signal number = %d%s\n", WTERMSIG(status),
#ifdef WCOREDUMP
           WCOREDUMP(status) ? " (core file generated)" : "");
#else
           "");
#endif
  } else if (WIFSTOPPED(status)) {
    printf("child stopped, signal number = %d\n", WSTOPSIG(status));
  }
}
