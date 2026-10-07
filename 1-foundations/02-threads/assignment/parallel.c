/*
 * parallel.c - Module 02 assignment, part A.  Replace every TODO.
 * Build & test:  make test
 *
 * Suggested structure: one argument struct per thread holding the input
 * slice and a result field, an array of nthreads such structs (malloc it,
 * nthreads can be large), create all threads, join all threads, combine.
 */
#include "parallel.h"

#include <pthread.h>
#include <stdlib.h>

void chunk_bounds(size_t n, size_t nthreads, size_t idx, size_t *begin, size_t *end)
{
  /* TODO */
  *begin = 0;
  *end = 0;
}

long long par_sum(const int *a, size_t n, size_t nthreads)
{
  /* TODO */
  return 0;
}

int par_max_index(const int *a, size_t n, size_t nthreads, size_t *out)
{
  /* TODO: careful with ties at chunk borders - combine the per-thread
   * results so that the smallest index wins. */
  return -1;
}

size_t par_count_if(const int *a, size_t n, size_t nthreads,
                    int (*pred)(int x, void *ctx), void *ctx)
{
  /* TODO */
  return 0;
}
