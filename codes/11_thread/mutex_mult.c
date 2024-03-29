/*
 * 使用散列链表存储信号量
 * 每个id对应一个信号量
*/
#include<stdlib.h>
#include<pthread.h>

#define NHASH 29
#define HASH(id) (((unsigned long)id)%NHASH)

struct foo
{
    int f_count;
    pthread_mutex_t f_lock;
    int f_id;
    struct foo *f_next;
    /* something else */
};

struct foo *fh[NHASH]; // all NULL
pthread_mutex_t hashlock = PTHREAD_MUTEX_INITIALIZER;

struct foo *
foo_alloc(int id)
{
    struct foo *fp;
    int idx;

    if((fp = malloc(sizeof(struct foo))) != NULL){
        fp->f_count = 1;
        fp->f_id = id;
        if(pthread_mutex_init(&fp->f_lock, NULL) != 0){
            free(fp);
            return(NULL);
        }
        idx = HASH(id);
        pthread_mutex_lock(&hashlock);
        // insert
        fp->f_next = fh[idx];
        fh[idx] = fp;
        // 有关互斥量和链表结构建立完成，放入了hash table，
        // 这时候可以释放全局hash锁，但是新锁可能需要其他初始化步骤
        // 因此换了个锁
        pthread_mutex_lock(&fp->f_lock);
        pthread_mutex_unlock(&hashlock);
        // 新锁继续初始化
        // ....
        pthread_mutex_unlock(&fp->f_lock);
    }
}

void foo_hold(struct foo *fp){
    pthread_mutex_lock(&fp->f_lock);
    fp->f_count++;
    pthread_mutex_unlock(&fp->f_lock);
}

struct foo * foo_find(int id) {
    struct foo *fp;

    pthread_mutex_lock(&hashlock);
    for(fp = fh[HASH(id)]; fp != NULL; fp = fp->f_next) {
        if (fp->f_id == id) {
            foo_hold(fp);
            break;
        }
    }
    pthread_mutex_unlock(&hashlock);
    return(NULL);
}

void foo_rele(struct foo *fp) {
    struct foo *tfp;
    int idx;

    pthread_mutex_lock(&fp->f_lock);
    if(fp->f_count == 1) {
        // 用确定的加锁顺序确保不会出现死锁
        pthread_mutex_unlock(&fp->f_lock);
        pthread_mutex_lock(&hashlock);
        pthread_mutex_lock(&fp->f_lock);
        // recheck
        if(fp->f_count != 1) {
            fp->f_count--;
            pthread_mutex_unlock(&fp->f_lock);
            pthread_mutex_unlock(&hashlock);
            return;
        }
        // remove mutex 散列表中的删除
        idx = HASH(fp->f_id);
        tfp = fh[idx];
        if(tfp == fp) {
            fh[idx] = fp->f_next;
        } else {
            while(tfp->f_next != fp) {
                tfp = tfp->f_next;
            }
            tfp->f_next = fp->f_next;
        }
        pthread_mutex_unlock(&fp->f_lock);
        pthread_mutex_unlock(&hashlock);
        pthread_mutex_destroy(&fp->f_lock);
        free(fp);
    } else {
        fp->f_count--;
        pthread_mutex_unlock(&hashlock);
    }
}