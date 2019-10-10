#include "thread_pool.h"

// 私有函数
int _poll_lock_init(thread_pool_t *thread_pool);  // 初始化锁
int _pool_free(thread_pool_t *thread_pool);       // 释放锁和线程池内存
int _pool_worker_alive(pthread_t tid);  // 判断worker线程是否存活
void *thr_worker(void *arg);            // 线程池中的线程
void *thr_manager(void *arg);           // 负责监视线程池的管理者

thread_pool_t *pool_create(size_t sz_init, size_t sz_min, size_t sz_max,
                           size_t task_mx) {
  int ret, i;
  thread_pool_t *thread_pool_ptr;
  printf("creating thread pool ...\n");
  do {
    if (sz_init < sz_min) {
      printf("sz_init < sz_min\n");
      break;
    } else if (sz_init > sz_max) {
      printf("sz_init > sz_max\n");
      break;
    }
    // -----------------------------------------------------
    // 创建线程池空间
    //      初始化互斥锁与条件变量
    //      为线程池分配空间，包括任务队列与工作线程队列
    //      设置任务队列与工作队列的初始状态
    // -----------------------------------------------------
    if ((thread_pool_ptr = (thread_pool_t *)malloc(sizeof(thread_pool_t))) ==
        NULL) {
      printf("pool_ptr malloc error\n");
      break;
    }
    // 线程池状态
    thread_pool_ptr->status = 1;
    // 互斥锁与条件变量
    if ((ret = _poll_lock_init(thread_pool_ptr)) != 0) {
      printf("lock or cond init error, exit code = %d\n", ret);
      break;
    }
    // 任务队列
    if ((thread_pool_ptr->tasks = (task_t *)malloc(sizeof(task_t) * task_mx)) ==
        NULL) {
      printf("tasks queue malloc error\n");
      break;
    }
    bzero(thread_pool_ptr->tasks, sizeof(task_t) * task_mx);
    thread_pool_ptr->queue_head = 0;
    thread_pool_ptr->queue_tail = 0;
    thread_pool_ptr->n_task = 0;
    thread_pool_ptr->queue_sz = task_mx;
    // 线程池
    if ((thread_pool_ptr->workers =
             (pthread_t *)malloc(sizeof(pthread_t) * sz_max)) == NULL) {
      printf("threads queue malloc error\n");
      break;
    }
    bzero(thread_pool_ptr->workers, sizeof(pthread_t) * sz_max);
    thread_pool_ptr->count_act = 0;
    thread_pool_ptr->count_will_exit = 0;
    thread_pool_ptr->count_all = sz_init;
    thread_pool_ptr->limit_min = sz_min;
    thread_pool_ptr->limit_max = sz_max;

    printf("space allocated ...\n");

    // -----------------------------------------------------
    // 启动工作线程以及管理者线程
    //      启动的时候尚未开始监听，也不会有其他线程执行，因此对线程池的操作都不需要加锁
    // -----------------------------------------------------
    for (i = 0; i < sz_init; ++i) {
      if (pthread_create(thread_pool_ptr->workers + i, NULL, thr_worker,
                         (void *)thread_pool_ptr) != 0) {
        err_sys("worker thread create error");
        // TODO: retry
        break;
      }
    }
    if (pthread_create(&(thread_pool_ptr->mamger_tid), NULL, thr_manager,
                       (void *)thread_pool_ptr) != 0) {
      err_sys("manger thread create error");
      break;
    }

    printf("work and manger thread created ...\n");
    printf("Done\n");
    return thread_pool_ptr;
  } while (0);
  // 创建出错
  _pool_free(thread_pool_ptr);
  return NULL;
}

int pool_add(thread_pool_t *t_pool, void (*handler)(void *arg), void *arg) {
  printf("adding task to pool ...\n");
  int ret;
  pthread_mutex_lock(&(t_pool->lock));
  while ((t_pool->status) && (t_pool->queue_sz <= t_pool->n_task)) {
    if ((ret = pthread_cond_wait(&(t_pool->pool_not_full), &(t_pool->lock))) !=
        0) {
      printf("In pool_add, error while cond_wait, error num = %d\n", ret);
      return -1;
    }
  }
  do {
    if (t_pool->status == 0) break;
    // -----------------------------------------------------
    // 创建任务并加入到任务队列
    //      这里对于没有释放的arg需要进行释放
    //      （参数为使用者传入，如果使用者用了
    //          (void *)2;这类方式会导致这里出错）
    //      这里已经执行完成的任务直到使用的时候才free，会导致有部分处于"泄露"状态
    //      感觉使用者应该负责这个free操作
    // -----------------------------------------------------
    // if (t_pool->tasks[t_pool->queue_tail].arg != NULL) {
    //   free(t_pool->tasks[t_pool->queue_tail].arg);
    //   t_pool->tasks[t_pool->queue_tail].arg = NULL;
    // }
    t_pool->tasks[t_pool->queue_tail].handler = handler;
    t_pool->tasks[t_pool->queue_tail].arg = arg;
    t_pool->queue_tail = (t_pool->queue_tail + 1) % t_pool->queue_sz;
    t_pool->n_task++;

    printf("new task added\n");
    pthread_cond_signal(&(t_pool->pool_not_empty));
  } while (0);
  pthread_mutex_unlock(&(t_pool->lock));
  return 0;
}

int pool_destroy(thread_pool_t *t_pool) {
  int i;
  if (t_pool == NULL) {
    return -1;
  }
  pthread_mutex_lock(&(t_pool->lock));
  t_pool->status = 0;  // shutdown
  pthread_mutex_unlock(&(t_pool->lock));

  pthread_join(t_pool->mamger_tid, NULL);

  for (i = 0; i < t_pool->count_all; ++i) {
    pthread_cond_signal(&(t_pool->pool_not_empty));
  }
  for (i = 0; i < t_pool->limit_max; ++i) {
    pthread_join(t_pool->workers[i], NULL);
    // EINVA
  }
  _pool_free(t_pool);
}

// TODO: 线程的取消应该可以使用cancelstate的方式
void *thr_manager(void *arg) {
  int i, counter;
  float num_act, num_all;
  thread_pool_t *t_pool_ptr = (thread_pool_t *)arg;
  while (t_pool_ptr->status != 0) {
    sleep(POOL_MANAGER_LOOP);
    printf("manger waked ...\n");

    // TODO: 任务队列中任务数量也可以作为参考量
    pthread_mutex_lock(&(t_pool_ptr->count_lock));
    num_act = (float)t_pool_ptr->count_act;
    num_all = (float)t_pool_ptr->count_all;
    pthread_mutex_unlock(&(t_pool_ptr->count_lock));
    printf("thread pool state: %f/%f\n", num_act, num_all);
    if (num_act / num_all < 0.3) {
      // 减少线程池中线程数量
      pthread_mutex_lock(&(t_pool_ptr->lock));
      t_pool_ptr->count_will_exit = POOL_STEP;
      pthread_mutex_unlock(&(t_pool_ptr->lock));
      // TODO: 循环POOL_STEP次还是广播
      // pthread_cond_broadcast(&(t_pool_ptr->pool_not_empty));
      for (counter = 0; counter < POOL_STEP; ++counter) {
        pthread_cond_signal(&(t_pool_ptr->pool_not_empty));
      }
    } else if (num_act / num_all > 0.8) {
      // 增加线程池中线程数量，全部线程创建完成后再加入线程池
      for (counter = 0, i = 0;
           (counter < POOL_STEP) && (i < t_pool_ptr->limit_max); ++i) {
        if (t_pool_ptr->workers[i] == 0 ||
            !_pool_worker_alive(t_pool_ptr->workers[i])) {
          if (pthread_create(t_pool_ptr->workers + i, NULL, thr_worker,
                             (void *)t_pool_ptr) != 0) {
            err_sys("worker thread create error");
            break;
          }
          counter++;
        }
      }
      pthread_mutex_lock(&(t_pool_ptr->lock));
      t_pool_ptr->count_all += POOL_STEP;
      pthread_mutex_unlock(&(t_pool_ptr->lock));
    }
  }
  pthread_exit(NULL);
}

void *thr_worker(void *arg) {
  // 将取出任务和置为忙状态两个过程分离，避免死锁
  int ret;
  thread_pool_t *t_pool_ptr = (thread_pool_t *)arg;
  task_t task;
  pthread_mutex_lock(&(t_pool_ptr->lock));
  while (1) {
    // -----------------------------------------------------
    // 尝试获取线程池锁，
    //      处理manger要求工作线程退出
    //      处理线程池进入销毁状态
    // -----------------------------------------------------
    while ((t_pool_ptr->status) && (t_pool_ptr->n_task == 0)) {
      if ((ret = pthread_cond_wait(&(t_pool_ptr->pool_not_empty),
                                   &(t_pool_ptr->lock))) != 0) {
        printf("In thr_worker: error while cond_wait, error num = %d\n", ret);
        // TODO: error
      }
      // manger 线程要求部分工作线程退出
      if (t_pool_ptr->count_will_exit > 0) {
        t_pool_ptr->count_will_exit--;
        // 确保不会小于最小线程数
        if (t_pool_ptr->count_all > t_pool_ptr->limit_min) {
          t_pool_ptr->count_all--;
          pthread_mutex_unlock(&(t_pool_ptr->lock));
          printf("thread: %lu exited", pthread_self());
          pthread_exit(NULL);
        }
      }
    }
    // 线程池进入销毁状态
    if (t_pool_ptr->status == 0) {
      pthread_mutex_unlock(&(t_pool_ptr->lock));
      break;
    }
    // -----------------------------------------------------
    // 线程从任务队列中获取任务
    //      这里不能使用指针，因为任务分离出来后，任务队列随时会被覆盖
    //      应该直接做一份task的拷贝
    // -----------------------------------------------------
    printf("thread: %lu got a task\n", pthread_self());
    task.handler = t_pool_ptr->tasks[t_pool_ptr->queue_head].handler;
    task.arg = t_pool_ptr->tasks[t_pool_ptr->queue_head].arg;
    t_pool_ptr->queue_head =
        (t_pool_ptr->queue_head + 1) % t_pool_ptr->queue_sz;
    t_pool_ptr->n_task--;
    pthread_cond_signal(&(t_pool_ptr->pool_not_full));
    pthread_mutex_unlock(&(t_pool_ptr->lock));
    // -----------------------------------------------------
    // 线程进入处理任务状态
    //      先记录自己进入忙状态，之后执行工作
    //      完成工作后同样需要记录
    // -----------------------------------------------------
    pthread_mutex_lock(&(t_pool_ptr->count_lock));
    t_pool_ptr->count_act++;
    printf("thread: %lu activated\n", pthread_self());
    pthread_mutex_unlock(&(t_pool_ptr->count_lock));
    // 执行指定的函数
    (task.handler)(task.arg);

    pthread_mutex_lock(&(t_pool_ptr->count_lock));
    t_pool_ptr->count_act--;
    pthread_mutex_unlock(&(t_pool_ptr->count_lock));
    printf("thread: %lu finished work\n", pthread_self());
  }
  pthread_exit(NULL);
}

int _poll_lock_init(thread_pool_t *thread_poll) {
  int ret;
  do {
    if ((ret = pthread_mutex_init(&(thread_poll->lock), NULL)) != 0) {
      break;
    }
    if ((ret = pthread_mutex_init(&(thread_poll->count_lock), NULL)) != 0) {
      break;
    }
    if ((ret = pthread_cond_init(&(thread_poll->pool_not_empty), NULL)) != 0) {
      break;
    }
    if ((ret = pthread_cond_init(&(thread_poll->pool_not_full), NULL)) != 0) {
      break;
    }
  } while (0);
  return ret;
}

int _pool_free(thread_pool_t *thread_pool) {
  if (thread_pool == NULL) return -1;
  int i, ret;
  do {
    pthread_mutex_lock(&(thread_pool->lock));
    if ((ret = pthread_mutex_destroy(&(thread_pool->lock))) != 0) {
      break;
    }
    pthread_mutex_lock(&(thread_pool->count_lock));
    if ((ret = pthread_mutex_destroy(&(thread_pool->count_lock))) != 0) {
      break;
    }
    if ((ret = pthread_cond_destroy(&(thread_pool->pool_not_empty))) != 0) {
      break;
    }
    if ((ret = pthread_cond_destroy(&(thread_pool->pool_not_full))) != 0) {
      break;
    }
  } while (0);
  if (thread_pool->tasks != NULL) {
    free(thread_pool->tasks);
  }
  if (thread_pool->workers != NULL) {
    free(thread_pool->workers);
  }
  free(thread_pool);
  thread_pool = NULL;
  return ret;
}

int _pool_worker_alive(pthread_t tid) {
  int kill_rc = pthread_kill(tid, 0);
  if (kill_rc == ESRCH) {
    return 0;
  }
  return 1;
}