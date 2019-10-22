#ifndef THREAD_POOL_H_INCLUDED
#define THREAD_POOL_H_INCLUDED

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <strings.h>
#include <sys/socket.h>
#include <time.h>
#include "../include/apue.h"

// 线程池配置
#define POOL_INIT_SIZE 10                  // 初始大小
#define POOL_SIZE_MAX POOL_INIT_SIZE * 10  // 最大
#define POOL_SIZE_MIN POOL_INIT_SIZE       // 最小
#define POOL_STEP 5  // 增加或者减少线程数量的步长
#define POOL_MANAGER_LOOP \
  5  // 管理者线程定时周期 - 在一定的循环周期后再唤醒进行处理
#define POOL_TASK_QUEUE_SIZE 2048  // 任务队列大小

// ------------------------------------------------------------------------
// 任务队列的任务结构体与线程池结构体
//  - 对于任务队列，需要一个处理函数交给线程执行，传入参数为泛型指针 
//              => 这里线程并不是handler，而是线程池中不断取走任务，执行任务的线程
//  - 线程池结构体
//    - 需要对线程池本身进行管理
//    - 需要对任务队列进行管理
// ------------------------------------------------------------------------

typedef struct _task {
  void (*handler)(void *arg);
  void *arg;
} task_t;

typedef struct _thread_pool {
  // 线程池状态
  int status;  // working - 1; shutdown - 0
  // 互斥锁与条件变量
  pthread_mutex_t
      lock;  // 线程池互斥锁
             // 除了count_act，对线程池整个的锁，使用这个的时候不更改count_act
  pthread_mutex_t
      count_lock;  // count_act的锁 活跃线程会较频繁地使用，因此单独加一个锁
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
//      创建线程池，成功返回线程池指针，出错返回NULL
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