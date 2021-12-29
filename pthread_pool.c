#include "pthread_pool.h"
#include "error_util.h"
#include <memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *threadLoop(void *arg);
void ThreadPool_threadWait(struct ThreadPool *threadPool);
void ThreadPool_threadBroadcast(struct ThreadPool *threadPool);
void ThreadPool_threadSignal(struct ThreadPool *threadPool);
void ThreadPool_destroy(struct ThreadPool *threadPool);
void ThreadPool_lock(struct ThreadPool *threadPool);
void ThreadPool_unlock(struct ThreadPool *threadPool);

void *threadLoop(void *arg) {
  struct TaskQueue *queue;
  struct ThreadPool *threadPool = (struct ThreadPool *)arg;

  ThreadPool_lock(threadPool);
  threadPool->readyThreads++;
  ThreadPool_threadBroadcast(threadPool);
  ThreadPool_unlock(threadPool);

  while (true) {
    ThreadPool_lock(threadPool);

    while (!threadPool->destroyed && threadPool->queue == NULL) {
      ThreadPool_threadWait(threadPool);
    }

    if (threadPool->destroyed &&
        (threadPool->tasksCount == 0 || threadPool->queue == NULL)) {
      ThreadPool_unlock(threadPool);
      break;
    }

    queue = threadPool->queue;
    threadPool->queue = queue->next;
    threadPool->end = (queue == threadPool->end ? NULL : threadPool->end);
    ThreadPool_unlock(threadPool);

    queue->fn(queue->arg);

    ThreadPool_lock(threadPool);
    threadPool->tasksCount--;
    ThreadPool_threadSignal(threadPool);
    ThreadPool_unlock(threadPool);
  }

  return EXIT_SUCCESS;
}

void ThreadPool_addTask(struct ThreadPool *threadPool, void *(*fn)(void *),
                        void *arg) {
  struct TaskQueue *taskQueue =
      (struct TaskQueue *)malloc(sizeof(struct TaskQueue));

  taskQueue->fn = fn;
  taskQueue->arg = arg;
  taskQueue->next = NULL;

  ThreadPool_lock(threadPool);
  if (threadPool->end != NULL)
    threadPool->end->next = taskQueue;
  if (threadPool->queue == NULL)
    threadPool->queue = taskQueue;

  threadPool->end = taskQueue;
  threadPool->tasksCount++;

  ThreadPool_threadSignal(threadPool);
  ThreadPool_unlock(threadPool);
}

struct ThreadPool *ThreadPool_new() {
  int error;
  struct ThreadPool *threadPool = malloc(sizeof(struct ThreadPool));
  memset(threadPool, 0, sizeof(struct ThreadPool));
  threadPool->destroyed = false;
  threadPool->tasksCount = 0;

  error = pthread_mutex_init(&threadPool->mutex, NULL);
  if (error != 0)
    errExit("pthread_mutex_init");

  error = pthread_cond_init(&threadPool->cond, NULL);
  if (error != 0)
    errExit("pthread_cond_init");

  for (int i = 0; i < POOL_SIZE; i++) {
    error = pthread_create(&threadPool->pthreads[i], NULL, threadLoop,
                           (void *)threadPool);
    if (error != 0)
      errExit("pthread_create");
  }

  ThreadPool_lock(threadPool);
  while (threadPool->readyThreads != POOL_SIZE) {
    ThreadPool_threadWait(threadPool);
  }

  ThreadPool_unlock(threadPool);

  printf("Thread pool ready\n");

  return threadPool;
}

void ThreadPool_shutdown(struct ThreadPool *threadPool) {
  int error;

  threadPool->destroyed = true;
  ThreadPool_threadBroadcast(threadPool);

  for (int i = 0; i < POOL_SIZE; i++) {
    error = pthread_join(threadPool->pthreads[i], NULL);
    if (error != 0)
      errExit("pthread_join");
  }

  ThreadPool_destroy(threadPool);
}

void ThreadPool_destroy(struct ThreadPool *threadPool) {
  int error;
  error = pthread_mutex_destroy(&threadPool->mutex);
  if (error != 0)
    errExit("pthread_mutex_destroy");

  error = pthread_cond_destroy(&threadPool->cond);
  if (error != 0)
    errExit("pthread_cond_destroy");

  printf("Thread pool destroyed\n");

  free(threadPool);
}

void ThreadPool_lock(struct ThreadPool *threadPool) {
  int error = pthread_mutex_lock(&threadPool->mutex);
  if (error != 0)
    errExit("pthread_mutex_lock");
}

void ThreadPool_unlock(struct ThreadPool *threadPool) {
  int error = pthread_mutex_unlock(&threadPool->mutex);
  if (error != 0)
    errExit("pthread_mutex_unlock");
}

void ThreadPool_threadWait(struct ThreadPool *threadPool) {
  int error = pthread_cond_wait(&threadPool->cond, &threadPool->mutex);
  if (error != 0)
    errExit("pthread_cond_wait");
}

void ThreadPool_threadBroadcast(struct ThreadPool *threadPool) {
  int error = pthread_cond_broadcast(&threadPool->cond);
  if (error != 0)
    errExit("pthread_cond_broadcast");
}

void ThreadPool_threadSignal(struct ThreadPool *threadPool) {
  int error = pthread_cond_signal(&threadPool->cond);
  if (error != 0)
    errExit("pthread_cond_signal");
}
