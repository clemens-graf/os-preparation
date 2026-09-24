/*
 * Reference solution for Module 03, part B.
 *
 * Races in the original:
 *  - count++ and sum += x: lost updates.
 *  - min/max: check-then-act ("x < min, so min = x"); another thread can
 *    set a smaller min in between, which then gets overwritten.
 *  - the four fields form ONE invariant (they describe the same samples);
 *    a snapshot taken between "count++" and "sum += x" is inconsistent.
 *
 * Fix: one lock for the whole struct, held for the complete update and for
 * the complete snapshot. A separate atomic per field would fix the lost
 * updates but NOT the consistency between the fields - the invariant spans
 * several variables, so the lock must too.
 */
#include "stats.h"

void stats_init(stats_t *s)
{
  s->count = 0;
  s->sum = 0;
  s->min = 0;
  s->max = 0;
  pthread_mutex_init(&s->lock, NULL);
}

void stats_destroy(stats_t *s)
{
  pthread_mutex_destroy(&s->lock);
}

void stats_add(stats_t *s, int x)
{
  pthread_mutex_lock(&s->lock);
  if (s->count == 0 || x < s->min)
    s->min = x;
  if (s->count == 0 || x > s->max)
    s->max = x;
  s->count++;
  s->sum += x;
  pthread_mutex_unlock(&s->lock);
}

void stats_snapshot(stats_t *s, long *count, long long *sum, int *min, int *max)
{
  pthread_mutex_lock(&s->lock);
  *count = s->count;
  *sum = s->sum;
  *min = s->min;
  *max = s->max;
  pthread_mutex_unlock(&s->lock);
}
