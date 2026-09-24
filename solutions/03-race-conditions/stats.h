/* Reference solution for Module 03, part B (header). */
#pragma once
#include <pthread.h>

typedef struct {
  long count;
  long long sum;
  int min, max;
  pthread_mutex_t lock;   /* protects all four fields together */
} stats_t;

void stats_init(stats_t *s);
void stats_destroy(stats_t *s);
void stats_add(stats_t *s, int x);
void stats_snapshot(stats_t *s, long *count, long long *sum, int *min, int *max);
