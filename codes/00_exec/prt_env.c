#include <stdio.h>
#include <stdlib.h>

extern char **environ;

int main(void) {
  int i;

  for (i = 0; environ[i] != NULL; ++i) {
    printf("%s\n", environ[i]);
  }

  exit(0);
}