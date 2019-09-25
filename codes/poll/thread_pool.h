#ifndef THREAD_POOL_H_INCLUDED
#define THREAD_POOL_H_INCLUDED

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <strings.h>
#include <sys/socket.h>
#include <time.h>
#include "../include/apue.h"

// 默认配置
#define POOL_INIT_SIZE 10                  // 初始大小
#define POOL_SIZE_MAX POOL_INIT_SIZE * 10  // 最大
#define POOL_SIZE_MIN POOL_INIT_SIZE       // 最小
#define POOL_STEP 5                        // 增加或者减少线程的步长
#define POOL_MANAGER_LOOP 10               // 管理者线程定时周期
#define POOL_TASK_QUEUE_SIZE 2048          // 任务队列大小

// ------------------------------------------------------------------------
// 任务队列的任务结构体与线程池结构体
// ------------------------------------------------------------------------
typedef struct _task {
  void (*handler)(void *arg);
  void *arg;
} task_t;

typedef struct _thread_pool {
  // 线程池状态
  int status;  // working - 1; shutdown - 0
  // 互斥锁与条件变量
  pthread_mutex_t lock;           // 线程池互斥锁
  pthread_mutex_t count_lock;     // count_act的锁
  pthread_cond_t pool_not_empty;  // 线程池非空信号量
  pthread_cond_t pool_not_full;   // 线程池非满信号量

  // 任务队列
  task_t *tasks;  // 任务队列 -- 循环队列
  int queue_head;
  int queue_tail;
  int n_task;    // 队列中任务数量
  int queue_sz;  // 任务队列size

  // 线程池管理
  pthread_t mamger_tid;  // 管理者线程
  pthread_t *workers;    // 线程池中的线程
  int count_act;         // 活跃线程计数
  int count_all;         // 线程池中全部线程数
  int count_will_exit;   // 即将退出线程数量
  int limit_max;         // 最大最小线程限制个数
  int limit_min;
} thread_pool_t;

// ------------------------------------------------------------------------
// 线程池操作函数
// create:
//      创建线程池
// add：
//      向线程池添加任务
// destroy：
//      线程池销毁
// ------------------------------------------------------------------------
thread_pool_t *pool_create(size_t sz_init, size_t sz_min, size_t sz_max,
                           size_t task_mx);
int pool_add(thread_pool_t *t_pool, void (*handler)(void *arg), void *arg);
int pool_destroy(thread_pool_t *t_pool);

#endif