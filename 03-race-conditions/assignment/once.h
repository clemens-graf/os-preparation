/*
 * once.h - Module 03 assignment, part C: build your own pthread_once.
 *
 *   static my_once_t once = MY_ONCE_INIT;
 *   my_once(&once, init_everything);
 *
 * Guarantees:
 *   - init runs exactly once, no matter how many threads call my_once
 *     concurrently;
 *   - NO caller returns from my_once before init has COMPLETED (a caller
 *     that arrives while another thread is still inside init must wait).
 *
 * If you add fields to my_once_t, update MY_ONCE_INIT to initialise them
 * (e.g. PTHREAD_MUTEX_INITIALIZER for a mutex).
 */
#pragma once
#include <pthread.h>

typedef struct {
  int done;
  /* TODO */
} my_once_t;

#define MY_ONCE_INIT {0}

void my_once(my_once_t *o, void (*init)(void));
