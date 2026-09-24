/* Reference solution for Module 03, part C (header). */
#pragma once
#include <pthread.h>
#include <stdatomic.h>

typedef struct {
  atomic_int done;          /* set (release) only after init has returned */
  pthread_mutex_t lock;     /* serialises the threads that race to run init */
} my_once_t;

#define MY_ONCE_INIT {0, PTHREAD_MUTEX_INITIALIZER}

void my_once(my_once_t *o, void (*init)(void));
