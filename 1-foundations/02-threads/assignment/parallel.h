/*
 * parallel.h - Module 02 assignment, part A.  Implement in parallel.c.
 *
 * Split work over threads WITHOUT any locks: every thread works on its own
 * slice of the input and writes only to its own result slot; the calling
 * thread combines the slots after pthread_join. This "partition, then
 * combine" pattern avoids races by design - no shared writes, no locks.
 *
 * Rules (checked by the tests):
 *   - every par_* function creates exactly `nthreads` threads with
 *     pthread_create and joins every one of them before returning;
 *   - the element work (the predicate in par_count_if) happens in the
 *     worker threads, not in the calling thread;
 *   - no global/static variables, no mutexes, no atomics.
 */
#pragma once
#include <stddef.h>

/* Chunk `idx` (0 <= idx < nthreads) of the index range [0, n) is
 * [*begin, *end). Chunks are contiguous, in order, cover [0, n) exactly,
 * and their sizes differ by at most one: the first (n % nthreads) chunks
 * get one extra element.  Example n = 10, nthreads = 3:
 *     [0,4)  [4,7)  [7,10)
 * With n < nthreads, the last chunks are empty ([n, n)). */
void chunk_bounds(size_t n, size_t nthreads, size_t idx, size_t *begin, size_t *end);

/* Sum of a[0..n). nthreads >= 1. */
long long par_sum(const int *a, size_t n, size_t nthreads);

/* Index of the maximum element of a[0..n); if the maximum occurs several
 * times, the SMALLEST such index. Returns 0 and stores the index in *out,
 * or returns -1 if n == 0. */
int par_max_index(const int *a, size_t n, size_t nthreads, size_t *out);

/* Number of elements x with pred(x, ctx) != 0.  pred is called exactly
 * once per element, from a worker thread. */
size_t par_count_if(const int *a, size_t n, size_t nthreads,
                    int (*pred)(int x, void *ctx), void *ctx);
