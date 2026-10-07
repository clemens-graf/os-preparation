/*
 * Reference solution for Module 02, part A.
 */
#include "parallel.h"

#include <pthread.h>
#include <stdlib.h>

void chunk_bounds(size_t n, size_t nthreads, size_t idx, size_t *begin, size_t *end)
{
  size_t base = n / nthreads;
  size_t extra = n % nthreads;          /* the first `extra` chunks get +1 */
  *begin = idx * base + (idx < extra ? idx : extra);
  *end = *begin + base + (idx < extra ? 1 : 0);
}

/* One struct per worker: its slice of the input and its own result slot.
 * Workers never write anywhere else, so no locking is needed; pthread_join
 * guarantees that main sees everything a worker wrote before it ended. */
struct slice {
  const int *a;
  size_t begin, end;
  int (*pred)(int, void *);
  void *ctx;
  /* results */
  long long sum;
  size_t count;
  size_t max_idx;     /* only meaningful if begin < end */
};

typedef void *(*worker_fn)(void *);

/* Creates nthreads workers over [0, n), joins them all.
 * Returns the slice array (caller frees) or NULL on failure. */
static struct slice *run_workers(const int *a, size_t n, size_t nthreads, worker_fn fn,
                                 int (*pred)(int, void *), void *ctx)
{
  struct slice *s = calloc(nthreads, sizeof *s);
  pthread_t *tids = malloc(nthreads * sizeof *tids);
  if (!s || !tids) {
    free(s);
    free(tids);
    return NULL;
  }

  size_t started = 0;
  for (size_t i = 0; i < nthreads; i++) {
    s[i].a = a;
    s[i].pred = pred;
    s[i].ctx = ctx;
    chunk_bounds(n, nthreads, i, &s[i].begin, &s[i].end);
    if (pthread_create(&tids[i], NULL, fn, &s[i]) != 0)
      break;
    started++;
  }
  /* Join whatever was started - even on failure, never leave a thread
   * running that still points into memory we are about to free. */
  for (size_t i = 0; i < started; i++)
    pthread_join(tids[i], NULL);
  free(tids);

  if (started != nthreads) {
    free(s);
    return NULL;
  }
  return s;
}

static void *sum_worker(void *p)
{
  struct slice *s = p;
  long long sum = 0;              /* accumulate locally, publish once */
  for (size_t i = s->begin; i < s->end; i++)
    sum += s->a[i];
  s->sum = sum;
  return NULL;
}

long long par_sum(const int *a, size_t n, size_t nthreads)
{
  struct slice *s = run_workers(a, n, nthreads, sum_worker, NULL, NULL);
  if (!s)
    return 0;
  long long total = 0;
  for (size_t i = 0; i < nthreads; i++)
    total += s[i].sum;
  free(s);
  return total;
}

static void *max_worker(void *p)
{
  struct slice *s = p;
  if (s->begin == s->end)
    return NULL;
  size_t best = s->begin;
  for (size_t i = s->begin + 1; i < s->end; i++)
    if (s->a[i] > s->a[best])     /* strict >: keeps the first occurrence */
      best = i;
  s->max_idx = best;
  return NULL;
}

int par_max_index(const int *a, size_t n, size_t nthreads, size_t *out)
{
  if (n == 0)
    return -1;
  struct slice *s = run_workers(a, n, nthreads, max_worker, NULL, NULL);
  if (!s)
    return -1;
  /* Chunks are in index order, so scanning them in order with a strict >
   * again picks the smallest index among equal maxima. */
  int found = 0;
  size_t best = 0;
  for (size_t i = 0; i < nthreads; i++) {
    if (s[i].begin == s[i].end)
      continue;
    if (!found || a[s[i].max_idx] > a[best]) {
      best = s[i].max_idx;
      found = 1;
    }
  }
  free(s);
  *out = best;
  return 0;
}

static void *count_worker(void *p)
{
  struct slice *s = p;
  size_t c = 0;
  for (size_t i = s->begin; i < s->end; i++)
    c += s->pred(s->a[i], s->ctx) != 0;
  s->count = c;
  return NULL;
}

size_t par_count_if(const int *a, size_t n, size_t nthreads,
                    int (*pred)(int x, void *ctx), void *ctx)
{
  struct slice *s = run_workers(a, n, nthreads, count_worker, pred, ctx);
  if (!s)
    return 0;
  size_t total = 0;
  for (size_t i = 0; i < nthreads; i++)
    total += s[i].count;
  free(s);
  return total;
}
