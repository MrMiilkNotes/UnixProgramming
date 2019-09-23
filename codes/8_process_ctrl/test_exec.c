// exec
#include <sys/wait.h>
#include "../include/apue.h"

char *env_init[] = {"USER=unknown", "PATH=/tmp", NULL};

int main(void) {
  pid_t pid;
  const char *sub_pross_name = "echoall.exe";

  if ((pid = fork()) < 0) {
    err_sys("fork error");
  } else if (pid == 0) {
    if (execle("/home/out_world/Unix/codes/process_ctrl_8/echoall.exe",
               sub_pross_name, "nothing", (char *)0, env_init) < 0) {
      err_sys("execle error");
    }
  }

  // hang
  if (waitpid(pid, NULL, 0) < 0) err_sys("waitpid error");

  if ((pid = fork()) < 0) {
    err_sys("fork error");
  } else if (pid == 0) {
    // PATH 中没有当前路径:.
    // No such file or directory
    if (execlp(sub_pross_name, sub_pross_name, "nothing", (char *)0) < 0) {
      err_sys("execlp error");
    }
  }

  exit(0);
}