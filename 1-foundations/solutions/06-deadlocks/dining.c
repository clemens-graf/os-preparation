/*
 * Reference solution for Module 06, part B: resource ordering.
 *
 * Every philosopher picks up the LOWER-numbered of his two forks first.
 * All forks are thus acquired in increasing order, so the circular wait
 * "everyone holds the left fork, waits for the right" cannot form: the
 * philosopher whose forks are n-1 and 0 reaches for fork 0 first and
 * competes with philosopher 0 instead of completing the cycle.
 *
 * Alternatives: a "waiter" semaphore initialised to n-1 (at most n-1
 * philosophers may try to pick up forks, so at least one gets both -
 * breaks the cycle too), or Tanenbaum's monitor with per-philosopher
 * states, which only lets a HUNGRY philosopher eat when neither neighbour
 * is EATING (also avoids holding one fork while waiting: no hold-and-wait).
 */
#include "dining.h"

#include <stdlib.h>

int table_init(table_t *t, int n)
{
  t->n = n;
  t->forks = malloc((size_t)n * sizeof *t->forks);
  if (!t->forks)
    return -1;
  for (int i = 0; i < n; i++)
    pthread_mutex_init(&t->forks[i], NULL);
  return 0;
}

void table_destroy(table_t *t)
{
  for (int i = 0; i < t->n; i++)
    pthread_mutex_destroy(&t->forks[i]);
  free(t->forks);
}

static void forks_of(table_t *t, int id, int *low, int *high)
{
  int a = id, b = (id + 1) % t->n;
  *low = a < b ? a : b;
  *high = a < b ? b : a;
}

void pickup(table_t *t, int id)
{
  int low, high;
  forks_of(t, id, &low, &high);
  pthread_mutex_lock(&t->forks[low]);
  pthread_mutex_lock(&t->forks[high]);
}

void putdown(table_t *t, int id)
{
  int low, high;
  forks_of(t, id, &low, &high);
  pthread_mutex_unlock(&t->forks[high]);
  pthread_mutex_unlock(&t->forks[low]);
}
