/*
 * dining.h - Module 06 assignment, part B: dining philosophers.
 *
 * n philosophers (n >= 2) around a table; fork i lies between philosopher
 * i and philosopher (i+1) % n. Philosopher i needs forks i and (i+1) % n.
 *
 * Implement pickup/putdown so that
 *   - neighbours never eat at the same time,
 *   - there is no deadlock, however the threads are scheduled.
 * Any correct strategy is fine: resource ordering, a "waiter" that lets at
 * most n-1 philosophers try at once, or a monitor with states
 * THINKING/HUNGRY/EATING (Tanenbaum's solution). Add fields as needed.
 */
#pragma once
#include <pthread.h>

typedef struct {
  int n;
  pthread_mutex_t *forks;   /* one mutex per fork */
  /* TODO: whatever your strategy needs */
} table_t;

int table_init(table_t *t, int n);
void table_destroy(table_t *t);

void pickup(table_t *t, int id);   /* returns when philosopher id may eat */
void putdown(table_t *t, int id);  /* done eating */
