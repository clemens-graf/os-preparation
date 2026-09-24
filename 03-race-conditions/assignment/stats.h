/*
 * stats.h - Module 03 assignment, part B.
 *
 * Running statistics that many threads feed at once. As in part A, the
 * code in stats.c is correct single-threaded; make it thread-safe.
 * Add fields to stats_t as needed.
 */
#pragma once
#include <pthread.h>

typedef struct {
  long count;
  long long sum;
  int min, max;      /* undefined while count == 0 */
  /* TODO: whatever you need for thread safety */
} stats_t;

void stats_init(stats_t *s);
void stats_destroy(stats_t *s);

/* Record one sample. */
void stats_add(stats_t *s, int x);

/* Copy all four values out as ONE consistent snapshot: they must describe
 * the same set of samples (e.g. never a count that includes a sample whose
 * value is not yet in sum). */
void stats_snapshot(stats_t *s, long *count, long long *sum, int *min, int *max);
