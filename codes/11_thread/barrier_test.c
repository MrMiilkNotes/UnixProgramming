#include "../include/apue.h"

// #include <bsd/stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <sys/time.h>

#define NTHR 12
#define NUMNUM 8000000L
#define TNUM (NUMNUM / NTHR)

long nums[NUMNUM];
long snums[NUMNUM];

pthread_barrier_t b;

#ifdef SOLARIS
#define heapsort qsort
#else
extern int heapsort(void *, size_t, size_t,
                    int (*)(const void *, const void *));
#endif

int complong(const void *arg1, const void *arg2) {
  long l1 = *(long *)arg1;
  long l2 = *(long *)arg2;

  if (l1 < l2) {
    return -1;
  } else if (l1 == l2) {
    return 0;
  } else {
    return 1;
  }
}

void *thr_fn(void *arg) {
  long idx = (long)arg;

  heapsort(&nums[idx], TNUM, sizeof(long), complong);
  pthread_barrier_wait(&b);

  return ((void *)0);
}

void merge() {
  long idx[NTHR];
  long num, i, j, minidx;

  for (i = 0; i < NTHR; ++i) {
    idx[i] = i * TNUM;
  }
  for (i = 0; i < NUMNUM; ++i) {
    num = LONG_MAX;
    for (j = 0; j < NTHR; ++j) {
      if (idx[j] < (j + 1) * TNUM && nums[idx[j]] < num) {
        minidx = j;
        num = nums[idx[j]];
      }
    }
    snums[i] = nums[idx[minidx]];
    ++idx[minidx];
  }
}

int main(void) {
  unsigned long i;
  struct timeval start, end;
  long long startusec, endusec;
  double elapsed;
  int err;
  pthread_t tid;

  // create the initial set of numbers to sort
  srandom(2);
  for (i = 0; i < NUMNUM; ++i) {
    nums[i] = random();
  }

  // create 8 threads to work
  gettimeofday(&start, NULL);
  pthread_barrier_init(&b, NULL, NTHR + 1);
  for (i = 0; i < NTHR; ++i) {
    err = pthread_create(&tid, NULL, thr_fn, (void *)(i * TNUM));
    if (err != 0) {
      err_exit(err, "can't create thread");
    }
  }
  pthread_barrier_wait(&b);
  merge();
  gettimeofday(&end, NULL);

  // sort information
  startusec = start.tv_sec * 1000000 + start.tv_usec;
  endusec = end.tv_sec * 1000000 * end.tv_usec;
  elapsed = (double)(endusec - startusec) / 1000000.0;
  printf("sort took %.4f seconds\n", elapsed);
  exit(0);
}