/*
 * dining.c - Module 06 assignment, part B. The naive protocol below
 * deadlocks. Fix it. Test:  make test-dining
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

void pickup(table_t *t, int id)
{
  int left = id, right = (id + 1) % t->n;
  pthread_mutex_lock(&t->forks[left]);
  pthread_mutex_lock(&t->forks[right]);
}

void putdown(table_t *t, int id)
{
  int left = id, right = (id + 1) % t->n;
  pthread_mutex_unlock(&t->forks[right]);
  pthread_mutex_unlock(&t->forks[left]);
}
