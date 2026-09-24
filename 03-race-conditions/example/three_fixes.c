/*
 * three_fixes.c - three correct ways to count with many threads, timed.
 *
 *  (a) mutex around counter++          correct, simple, slowest under contention
 *  (b) atomic_fetch_add(&counter, 1)   correct, one indivisible CPU instruction
 *                                      (`lock xadd` on x86); still fights over
 *                                      the same cache line
 *  (c) count locally, add once at the end
 *                                      correct and fastest: threads share nothing
 *                                      while working (module 02's pattern)
 *
 * Lesson: first make it correct, then avoid sharing instead of optimising
 * the lock.
 */
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

#define THREADS 4
#define ITERATIONS 2000000

static long counter_mutex;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static atomic_long counter_atomic;
static long counter_local_sum;
static pthread_mutex_t sum_lock = PTHREAD_MUTEX_INITIALIZER;

static void *with_mutex(void *arg)
{
  (void)arg;
  for (int i = 0; i < ITERATIONS; i++) {
    pthread_mutex_lock(&lock);
    counter_mutex++;                    /* critical section: one thread at a time */
    pthread_mutex_unlock(&lock);
  }
  return NULL;
}

static void *with_atomic(void *arg)
{
  (void)arg;
  for (int i = 0; i < ITERATIONS; i++)
    atomic_fetch_add_explicit(&counter_atomic, 1, memory_order_relaxed);
  return NULL;
}

static void *with_local(void *arg)
{
  (void)arg;
  long mine = 0;                        /* private: no sharing, no race */
  for (int i = 0; i < ITERATIONS; i++)
    mine++;
  pthread_mutex_lock(&sum_lock);        /* one short critical section per thread */
  counter_local_sum += mine;
  pthread_mutex_unlock(&sum_lock);
  return NULL;
}

static double run(void *(*fn)(void *))
{
  struct timespec a, b;
  pthread_t t[THREADS];
  clock_gettime(CLOCK_MONOTONIC, &a);
  for (int i = 0; i < THREADS; i++)
    pthread_create(&t[i], NULL, fn, NULL);
  for (int i = 0; i < THREADS; i++)
    pthread_join(t[i], NULL);
  clock_gettime(CLOCK_MONOTONIC, &b);
  return (double)(b.tv_sec - a.tv_sec) * 1e3 + (double)(b.tv_nsec - a.tv_nsec) / 1e6;
}

int main(void)
{
  long expected = (long)THREADS * ITERATIONS;
  double t1 = run(with_mutex);
  double t2 = run(with_atomic);
  double t3 = run(with_local);
  printf("(a) mutex : %ld / %ld  in %7.1f ms\n", counter_mutex, expected, t1);
  printf("(b) atomic: %ld / %ld  in %7.1f ms\n", atomic_load(&counter_atomic), expected, t2);
  printf("(c) local : %ld / %ld  in %7.1f ms\n", counter_local_sum, expected, t3);
  return 0;
}
