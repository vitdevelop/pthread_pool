#ifndef PTHREAD_POOL_H
#define PTHREAD_POOL_H

#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>
#define POOL_SIZE 5

struct TaskQueue {
  void *(*fn)(void *);
  void *arg; /* if needed */

  struct TaskQueue *next;
};

struct ThreadPool {
  pthread_t pthreads[POOL_SIZE];
  u_int32_t readyThreads;

  struct TaskQueue *queue;
  struct TaskQueue *end;
  u_int32_t tasksCount;

  pthread_mutex_t mutex;
  pthread_cond_t cond;

  bool destroyed;
};

struct ThreadPool *ThreadPool_new(void);

void ThreadPool_shutdown(struct ThreadPool *threadPool);

void ThreadPool_addTask(struct ThreadPool *threadPool, void *(*fn)(void *),
                        void *arg);

#endif
