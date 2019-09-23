#include <fcntl.h>
#include <memory.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

typedef struct smg {
  int id;
  char name[10];
} MSG;
