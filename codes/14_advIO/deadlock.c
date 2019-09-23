#include <fcntl.h>
#include "../include/apue.h"

int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len) {
  struct flock lock;

  lock.l_type = type;     /* F_RDLCK, F_WRLCK, F_UNLCK */
  lock.l_start = offset;  /* byte offset, relative to l_whence */
  lock.l_whence = whence; /* SEEK_SET, SEEK_CUR, SEEK_END */
  lock.l_len = len;       /* #bytes (0 means to EOF) */

  return (fcntl(fd, cmd, &lock));
}

static void lockbyte(const char *name, int fd, off_t offset) {
  if (writew_lock(fd, offset, SEEK_SET, 1) < 0) {
    err_sys("%s: writew_lock error", name);
  }
  printf("%s: got the lock, byte: %lld\n", name, (long long)offset);
}

int main(void) {
  int fd;
  pid_t pid;

  if (fd = creat("templock", FILE_MODE) < 0) {
    err_sys("creat error");
  }
  if (write(fd, "ab", 2) != 2) {
    err_sys("write error");
  }

  TELL_WAIT();
  if ((pid = fork()) < 0) {
    err_sys("fork error");
  } else if (pid == 0) {
    // child
    lockbyte("child", fd, 0);
    TELL_PARENT(getppid());
    WAIT_PARENT();
    lockbyte("child", fd, 1);
  } else {
    // parent
    lockbyte("parent", fd, 1);
    TELL_CHILD(pid);
    WAIT_CHILD();
    lockbyte("parent", fd, 0);
  }
  exit(0);
}