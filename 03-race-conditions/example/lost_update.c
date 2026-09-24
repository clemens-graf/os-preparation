/*
 * lost_update.c - the classic race: two threads doing counter++.
 *
 * counter++ looks like one step but is three machine instructions:
 *
 *     mov  rax, [counter]     ; LOAD   read memory into a register
 *     add  rax, 1             ; ADD    compute in the register
 *     mov  [counter], rax     ; STORE  write the register back
 *
 * If thread B loads between A's LOAD and A's STORE, both write back the
 * same value and one increment is lost. See it: `make asm`.
 *
 * `volatile` forces the compiler to really do LOAD/ADD/STORE each time
 * (otherwise it might keep counter in a register for the whole loop). It
 * does NOT make the increment atomic - volatile is not a synchronisation
 * tool. Run it a few times: the result changes every run.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define ITERATIONS 1000000

static volatile long counter = 0;

__attribute__((noinline)) static void increment(void)
{
  counter++;
}

static void *worker(void *arg)
{
  (void)arg;
  for (long i = 0; i < ITERATIONS; i++)
    increment();
  return NULL;
}

int main(int argc, char **argv)
{
  int nthreads = argc > 1 ? atoi(argv[1]) : 4;
  if (nthreads < 1 || nthreads > 64)
    nthreads = 4;

  pthread_t t[64];
  for (int i = 0; i < nthreads; i++)
    pthread_create(&t[i], NULL, worker, NULL);
  for (int i = 0; i < nthreads; i++)
    pthread_join(t[i], NULL);

  long expected = (long)nthreads * ITERATIONS;
  printf("%d threads x %d increments: counter = %ld, expected %ld, lost %ld (%.1f%%)\n",
         nthreads, ITERATIONS, counter, expected, expected - counter,
         100.0 * (double)(expected - counter) / (double)expected);
  return 0;
}
