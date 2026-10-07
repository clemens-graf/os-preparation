/*
 * bug1_loop_index.c - Module 02 assignment, part B (1/3).
 *
 * Expected output (exactly):
 *     squares: 0 1 4 9 16 25 36 49
 *
 * It prints something else (and ThreadSanitizer complains). Find the bug,
 * explain it in one sentence in a comment, fix it with the smallest
 * possible change. `make bugs` checks all three bug programs.
 */
#include <pthread.h>
#include <stdio.h>

#define N 8

static int squares[N];

static void *worker(void *arg)
{
  int id = *(int *)arg;
  if (id >= 0 && id < N)
    squares[id] = id * id;
  return NULL;
}

/*
 * FIX: every thread got the address of the SAME variable i, which main
 * keeps incrementing (and which is N after the loop). A worker reads *arg
 * whenever it happens to run, so it sees some later value of i: wrong
 * results, and a data race between main's i++ and the worker's read.
 * Give every thread its own, stable copy of its id.
 */
static int ids[N];

int main(void)
{
  pthread_t t[N];
  for (int i = 0; i < N; i++) {
    ids[i] = i;
    pthread_create(&t[i], NULL, worker, &ids[i]);
  }
  /* Alternative fix: pass the value inside the pointer,
   *   pthread_create(&t[i], NULL, worker, (void *)(intptr_t)i);
   * and read it with (int)(intptr_t)arg in the worker. */
  for (int i = 0; i < N; i++)
    pthread_join(t[i], NULL);

  printf("squares:");
  for (int i = 0; i < N; i++)
    printf(" %d", squares[i]);
  printf("\n");
  return 0;
}
