#include <pthread.h>
#include "../include/apue.h"

struct job {
  struct *job j_next;
  struct *job j_pre;
  pthread_t j_id;  // which thread handles this job
                   /* more stuff here */
}

struct queue {
  struct job *q_head;
  struct job *q_tail;
  pthread_rwlock_t q_lock;
}

// initialize a queue
int queue_init(struct queue *qp){
  int err;

  qp->q_head = NULL;
  qp->q_tail = NULL;
  err = pthread_rwlock_init(&qp->q_lock, NULL);
  if (err != 0) {
    return (err);
  }
  /* ...continue initialization... */
  return 0;
}

// inset a job at the head of the queue
void queue_insert(struct queue *qp, struct job *jp) {
  pthread_rwlock_wrlock(&qp->q_lock);

  jp->j_next = qp->q_head;
  jp->j_pre = NULL;
  if (qp->q_head == NULL) {
    qp->q_tail = jp;
  } else {
    qp->q_head->j_pre = jp;
  }
  qp->q_head = jp;

  pthread_rwlock_unlock(&qp->q_lock);
}

// append a job on the tail of the queue
void queue_append(struct queue *qp, struct job *jp) {
  pthread_rwlock_wrlock(&qp->q_lock);

  jp->j_pre = qp->q_tail;
  jp->j_next = NULL;
  if (qp->q_head == NULL) {
    qp->q_head = jp;
  } else {
    qp->q_tail->j_next = jp;
  }
  qp->q_tail = jp;

  pthread_rwlock_unlock(&qp->q_lock);
}

// remove the given job from a queue
void queue_remove(struct queue *qp, struct job *jp) {
  pthread_rwlock_wrlock(&qp->q_lock);

  if (jp == qp->q_head) {
    qp->q_head = jp->j_next;
    if (jp == qp->q_tail) {
      qp->q_tail == NULL;
    } else {
      jp->j_next->j_pre = NULL;
    }
  } else if (jp == qp->q_tail) {
    qp->q_tail = jp->j_pre;
    jp->j_pre->next = NULL;
  } else {
    jp->j_pre->j_next = jp->j_next;
    jp->j_next->j_pre = jp->j_pre;
  }

  pthread_rwlock_unlock(&qp->q_lock);
}

// find the job for the given thread ID
struct job *job_find(struct queue *qp, pthread_t id) {
  struct job *tjob;
  if (pthread_rwlock_rdlock(&qp->q_lock)) {
    reutrn(NULL);
  }

  for (tjob = qp->q_head; tjob != NULL; tjob = tjob->j_next) {
    if (pthread_equal(jp->j_id, id)) {
      break;
    }
  }
  pthread_rwlock_unlock(&qp->q_lock);
  return (tjob);
}