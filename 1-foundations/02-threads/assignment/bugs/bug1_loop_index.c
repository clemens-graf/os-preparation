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

int main(void)
{
  pthread_t t[N];
  for (int i = 0; i < N; i++)
    pthread_create(&t[i], NULL, worker, &i);
  for (int i = 0; i < N; i++)
    pthread_join(t[i], NULL);

  printf("squares:");
  for (int i = 0; i < N; i++)
    printf(" %d", squares[i]);
  printf("\n");
  return 0;
}
