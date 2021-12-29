#include "pthread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *hello(void *arg);
void *hello(void *arg) {
  int *num = (int *)arg;

  printf("Hello %d\n", *num);

  free(num);
  num = NULL;

  return NULL;
}

int main(int argc, char *argv[]) {
  struct ThreadPool *threadPool = ThreadPool_new();

  for (int i = 0; i < 1000; i++) {
    int *num = (int *)malloc(sizeof(int));
    *num = i;
    ThreadPool_addTask(threadPool, hello, num);
  }

  ThreadPool_shutdown(threadPool);

  exit(EXIT_SUCCESS);
}
