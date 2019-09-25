#include<time.h>
#include<strings.h>
#include<errno.h>
#include<pthread.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include"../include/apue.h"

// 线程池
#define POOL_INIT_SIZE 10                   // 初始大小
#define POOL_SIZE_MAX POOL_QUEUE_SIZE * 10  // 最大
#define POLL_SIZE_MIN POOL_QUEUE_SIZE       // 最小
#define POOL_STEP 5                         // 增加或者减少线程的步长
#define POOL_MANAGER_LOOP 10                // 管理者线程定时周期

typedef struct _task {
    void (*handler) (void *arg);
    void *arg;
} task_t;

typedef struct _thread_pool {
    // 线程池状态
    int status;                         // working - 1; shutdown - 0
    // 互斥锁与条件变量
    pthread_mutex_t lock;               // 线程池互斥锁
    pthread_mutex_t count_lock;         // 不同状态线程计数互斥锁
    pthread_cond_t pool_not_empty;      // 线程池非空信号量
    pthread_cond_t pool_not_full;       // 线程池非满信号量

    // 任务队列
    task_t tasks[TASK_QUEUE_SIZE];      // 任务队列 -- 循环队列
    int queue_head;
    int queue_tail;
    int n_task;                         // 队列中任务数量
    int queue_sz;                       // 任务队列size

    // 线程池管理
    thread_t mamger_tid;                // 管理者线程
    thread_t* workers;                  // 线程池中的线程
    int count_act;                      // 活跃线程计数
    int count_all;
    int count_will_exit;                // 即将退出线程数量
    int limit_max;                      // 最大最小线程个数
    int limit_min;
} thread_pool_t;

thread_pool_t* pool_create(size_t sz_init, size_t sz_min, size_t sz_max);
int pool_add(thread_pool_t* t_pool, void (&handler)(void *arg), void *arg);
int pool_destroy(thread_pool_t* t_pool);
int _poll_lock_init(thread_pool_t * thread_poll);       // 初始化锁
int _pool_free(thread_pool_t * thread_poll);            // 释放锁
int _pool_worker_alive(thread_t tid);                   // 判断worker线程是否存活

void* thr_worker(void *arg);
void* thr_manager(void *arg);

int main(void) {

}

// TODO: 出错时 空间的free
thread_pool_t* pool_create(size_t sz_init, size_t sz_min, size_t sz_max) {
    int ret, i;
    thread_pool_t* thread_poll_ptr;
    if (sz_init < sz_min) {
        printf("sz_init < sz_min\n");
        break;
    } else if (sz_init > sz_max) {
        printf("sz_init > sz_max\n");
        break;
    }

    do {
        if((thread_poll_ptr = (thread_pool_t *)malloc(sizeof(thread_pool_t))) == NULL) {
            printf("pool_ptr malloc error\n");
            break;
        }
        // 线程池状态
        thread_poll_ptr->status = 1;
        // 互斥锁与条件变量
        if((ret = _poll_lock_init(thread_poll_ptr)) != 0) {
            printf("lock or cond init error, exit code = %d\n", ret);
            break;
        }
        // 任务队列
        if((thread_poll_ptr;->tasks = (task_t *)malloc(sizeof(task_t) * sz_max)) == NULL) {
            printf("tasks queue malloc error\n");
            break;
        }
        thread_poll_ptr->queue_head = 0;
        thread_poll_ptr->queue_tail = 0;
        thread_pool_t->count = 0;
        thread_poll_ptr->queue_sz = sz_init;
        // 线程池
        if((thread_poll_ptr->workers = (thread_t *)malloc(sizeof(thread_t) * sz_max)) == NULL) {
            printf("threads queue malloc error\n");
            break;
        }
        bzero(thread_poll_ptr->workers, sizeof(thread_t) * sz_max);
        thread_poll_ptr->count_act = 0;
        thread_poll_ptr->count_will_exit = 0;
        thread_poll_ptr->count_all = sz_init;
        thread_poll_ptr->limit_min = sz_min;
        thread_poll_ptr->limit_max = sz_max;
        
        // 启动线程，启动的时候尚未开始监听
        for(i = 0; i < sz_init; ++i) {
            if(pthread_create(thread_poll_ptr->workers + i, NULL, thr_worker, (void *)thread_poll_ptr) != 0) {
                err_sys("worker thread create error");
                break;
            }
            ++thread_poll_ptr->count_act;
        }
        // 管理者
        if(pthread_create(&(thread_poll_ptr->mamger_tid), NULL, thr_manager, (void *)thread_poll_ptr) != 0) {
            err_sys("manger thread create error");
            break;
        }

        return thread_poll_ptr;
    } while(0);
    _pool_free(pool_ptr);
    return NULL;
}

int pool_add(thread_pool_t* t_pool, void (&handler)(void *arg), void *arg) {
    int ret;
    pthread_mutex_lock(&(t_pool->lock));
    while((!t_pool->shutdown) && (t_pool->queue_sz <= t_pool->n_task)) {
        // 非shutdown
        if((ret = pthread_cond_wait(&(t_pool->pool_not_full), &(t_pool->lock))) != 0) {
            printf("In pool_add, error while cond_wait, error num = %d\n", ret);
            return -1;
        }
    }
    do {
        if (t_pool->shutdown) break;  // unlock
        t_pool->tasks[t_pool->queue_tail].handler = handler;
        // TODO：如果arg是heap的数据？ 应该在指定的回调处理函数里面free
        t_pool->tasks[t_pool->queue_tail].arg = arg;
        t_pool->queue_tail = (t_pool->queue_tail + 1) % t_pool->queue_sz;
        t_pool->n_task++;

        pthread_cond_signal(&(t_pool->pool_not_empty));
    } while(0);
    pthread_mutex_unlock(&(t_pool->lock));
    return 0;
}

int pool_destroy(thread_pool_t* t_pool) {
    if (t_pool == NULL) {
        return -1;
    }
    pthread_mutex_lock(&(t_pool->lock));
    t_pool->status = 0; // shutdown
    pthread_mutex_unlock(&(t_pool->lock));

    pthread_join(t_pool->manger, NULL);

    for(i = 0; i < t_pool->count_all; ++i) {
        pthread_cond_signal(&(t_pool->pool_not_empty));
    }
    for(i = 0; i < t_pool->limit_max; ++i) {
        pthread_join(t_pool->workers[i]);
        // EINVA
    }
    _pool_free(t_pool);
}

// TODO: 线程的取消应该可以使用cancelstate的方式
void *thr_manager(void *arg) {
    int i, counter;
    float num_act, num_all;
    thread_pool_t *t_pool_ptr = (thread_pool_t *)arg;
    while(t_pool_ptr->status != 0) {
        sleep(POOL_MANAGER_LOOP)

        // TODO: 任务队列中任务数量
        pthread_mutex_lock(&(t_pool_ptr->count_lock));
        num_act = (float)t_pool_ptr->count_act;
        num_all = (float)t_pool_ptr->count_all;
        pthread_mutex_unlock(&(t_pool_ptr->count_lock));

        if (num_act / num_all < 0.3) {
            pthread_mutex_lock(&(t_pool_ptr->lock));
            // 减少线程池中线程数量
            // TODO: 循环POOL_STEP次还是广播
            t_pool_ptr->count_will_exit = POOL_STEP;
            // pthread_cond_broadcast(&(t_pool_ptr->pool_not_empty));
            for(counter=0; counter < POOL_STEP; ++counter) {
                pthread_cond_signal(&(t_pool_ptr->pool_not_empty));
            }
            pthread_mutex_unlock(&(t_pool_ptr->lock));
        } else if (num_act / num_all > 0.8) {
            pthread_mutex_lock(&(t_pool_ptr->lock));
            // 增加线程池中线程数量
            for(counter=0,i=0; (counter < POOL_STEP) && (i < t_pool_ptr->limit_max); ++i) {
                if(t_pool_ptr->workers[i] == 0 || !_pool_worker_alive(t_pool_ptr->workers[i])) {
                    if(pthread_create(thread_poll_ptr->workers + idx, NULL, thr_worker, (void *)thread_poll_ptr) != 0) {
                        err_sys("worker thread create error");
                        break;
                    }
                    counter++;
                }
            }
            t_pool_ptr->limit_max += POOL_STEP;
            t_poll_ptr->count_act += POOL_STEP;
            pthread_mutex_unlock(&(t_pool_ptr->lock));
        }
    }
    pthread_exit(NULL);
}

void* thr_worker(void *arg) {
    // 将取出任务和置为忙状态分离，避免死锁
    int ret;
    thread_pool_t * t_pool_ptr = (thread_pool_t *)arg;
    task_t task;
    pthread_mutex_lock(&(t_pool_ptr->lock));
    while(1) {
        while((!t_pool_ptr->shutdown) && (t_pool_ptr->n_task > 0)) {
            if((ret = pthread_cond_wait(&(t_pool_ptr->pool_not_empty), &(t_pool_ptr->lock))) != 0) {
                printf("In thr_worker: error while cond_wait, error num = %d\n", ret);
                // TODO: error
            }
            if (t_pool_ptr->count_will_exit > 0) {
                t_pool_ptr->count_will_exit--;
                // 确保不会小于最小线程数
                if(t_pool_ptr->count_all > t_pool_ptr->limit_min) {
                    t_pool_ptr->count_all--;
                    pthread_mutex_unlock(&(t_pool_ptr->lock));
                    pthread_exit(NULL);
                    // TODO: 线程池中的workers记录应该清除
                }
            }
        }
        if(t_pool_ptr->shutdown == 0) {
            break;
        }
        // 这里不能使用指针，因为任务分离出来后，任务队列随时会被覆盖
        // 应该直接做一份task的拷贝
        task.handler = t_pool_ptr->tasks[t_pool_ptr->queue_head].handler;
        task.arg = t_pool_ptr->tasks[t_pool_ptr->queue_head].arg;
        t_pool_ptr->queue_head = (t_pool_ptr->queue_head + 1) % t_pool_ptr->queue_sz;
        t_pool_ptr->n_task--;

        pthread_cond_signal(&(t_pool_ptr->pool_not_full));
        pthread_mutex_unlock(&(t_pool_ptr->lock));
        // 处理task
        pthread_mutex_lock(&(t_pool_ptr->count_lock));
        t_pool_ptr->count_act++;
        pthread_mutex_unlock(&(t_pool_ptr->count_lock));
        // 执行指定的函数
        (task.handler)(task.arg);

        pthread_mutex_lock(&(t_pool_ptr->count_lock));
        t_pool_ptr->count_act--;
        pthread_mutex_unlock(&(t_pool_ptr->count_lock));
    }
    pthread_exit(NULL);
}

int _poll_lock_init(thread_pool_t * thread_poll) {
    int ret;
    do {
        if((ret = pthread_mutex_init(&(thread_poll->lock), NULL)) != 0) {
            break;
        }
        if((ret = pthread_mutex_init(&(thread_poll->count_lock), NULL)) != 0) {
            break;
        }
        if((ret = pthread_cond_init(&(thread_poll->pool_not_empty), NULL)) != 0) {
            break;
        }
        if((ret = pthread_cond_init(&(thread_poll->pool_not_full), NULL)) != 0) {
            break;
        }
    } while(0);
    return ret;
}

int _pool_free(thread_pool_t * thread_poll) {
    int ret;
    do {
        if((ret = pthread_mutex_destroy(&(thread_poll->lock), NULL)) != 0) {
            break;
        }
        if((ret = pthread_mutex_destroy(&(thread_poll->count_lock), NULL)) != 0) {
            break;
        }
        if((ret = pthread_cond_destroy(&(thread_poll->pool_not_empty), NULL)) != 0) {
            break;
        }
        if((ret = pthread_cond_destroy(&(thread_poll->pool_not_full), NULL)) != 0) {
            break;
        }
    } while(0);
    return ret;
}

int _pool_worker_alive(thread_t tid) {
    int kill_rc = pthread_kill(tid, 0);
    if (kill_rc == ESRCH) {
        return false;
    }
    return true;
}