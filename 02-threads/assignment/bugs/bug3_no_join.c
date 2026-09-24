/*
 * bug3_no_join.c - Module 02 assignment, part B (3/3).
 *
 * Expected output (exactly):
 *     sum of 1..4000000 = 8000002000000
 *
 * Sometimes it prints the right number, often not, and ThreadSanitizer
 * reports a data race. Find the bug, explain it, fix it.
 */
#include <pthread.h>
#include <stdio.h>

#define N 4
#define PER_THREAD 1000000L

static long long partial[N];

static void *worker(void *arg)
{
  long id = (long)arg;
  long long sum = 0;
  for (long i = id * PER_THREAD + 1; i <= (id + 1) * PER_THREAD; i++)
    sum += i;
  partial[id] = sum;
  return NULL;
}

int main(void)
{
  pthread_t t[N];
  for (long i = 0; i < N; i++)
    pthread_create(&t[i], NULL, worker, (void *)i);

  long long total = 0;
  for (int i = 0; i < N; i++)
    total += partial[i];

  printf("sum of 1..%ld = %lld\n", N * PER_THREAD, total);
  return 0;
}
