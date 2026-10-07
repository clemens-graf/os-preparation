/*
 * stats.c - Module 03 assignment, part B. Correct single-threaded, racy
 * with threads. Find the races, comment them, fix them.
 */
#include "stats.h"

void stats_init(stats_t *s)
{
  s->count = 0;
  s->sum = 0;
  s->min = 0;
  s->max = 0;
}

void stats_destroy(stats_t *s)
{
  (void)s;
}

void stats_add(stats_t *s, int x)
{
  if (s->count == 0 || x < s->min)
    s->min = x;
  if (s->count == 0 || x > s->max)
    s->max = x;
  s->count++;
  s->sum += x;
}

void stats_snapshot(stats_t *s, long *count, long long *sum, int *min, int *max)
{
  *count = s->count;
  *sum = s->sum;
  *min = s->min;
  *max = s->max;
}
