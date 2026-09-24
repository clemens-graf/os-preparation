/*
 * philosophers.c - Dijkstra's dining philosophers.
 *
 * N philosophers sit around a table, one fork between each pair. To eat, a
 * philosopher needs both neighbouring forks. Naive protocol:
 *     take left fork; take right fork; eat; put both down.
 * If all N take their left fork at the same moment, everyone waits for the
 * right fork forever: a cycle of length N in the wait-for graph.
 *
 * Fix shown here (resource ordering): number the forks and always take the
 * LOWER-numbered fork first. Philosopher N-1's forks are N-1 and 0, so he
 * takes 0 first - the one "rebel" who breaks the cycle.
 *
 * `./philosophers` runs the naive version (forced into the deadlock), then
 * the ordered one. A watchdog detects the hang in the naive version.
 */
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define N 5
#define MEALS 1000

static pthread_mutex_t fork_lock[N];
static atomic_int meals[N];
static int ordered;
static pthread_barrier_t all_have_left_fork;

static void *philosopher(void *arg)
{
  int id = (int)(long)arg;
  int left = id, right = (id + 1) % N;
  int first = left, second = right;
  if (ordered && first > second) {        /* lower number first */
    first = right;
    second = left;
  }
  for (int i = 0; i < MEALS; i++) {
    pthread_mutex_lock(&fork_lock[first]);
    if (!ordered && i == 0)
      pthread_barrier_wait(&all_have_left_fork);   /* force the fatal moment */
    pthread_mutex_lock(&fork_lock[second]);
    atomic_fetch_add(&meals[id], 1);      /* eat */
    pthread_mutex_unlock(&fork_lock[second]);
    pthread_mutex_unlock(&fork_lock[first]);
  }
  return NULL;
}

static void run(int use_order)
{
  ordered = use_order;
  pthread_t t[N];
  for (int i = 0; i < N; i++) {
    pthread_mutex_init(&fork_lock[i], NULL);
    atomic_store(&meals[i], 0);
  }
  pthread_barrier_init(&all_have_left_fork, NULL, N);
  for (long i = 0; i < N; i++)
    pthread_create(&t[i], NULL, philosopher, (void *)i);

  /* watchdog: if no meal happens for a second, we are deadlocked */
  int last_total = -1;
  for (;;) {
    sleep(1);
    int total = 0;
    for (int i = 0; i < N; i++)
      total += atomic_load(&meals[i]);
    if (total == N * MEALS)
      break;
    if (total == last_total) {
      printf("%s: no progress for 1 s after %d meals -> DEADLOCK. Everyone holds\n"
             "   their left fork and waits for the right one:", use_order ? "ordered" : "naive", total);
      for (int i = 0; i < N; i++)
        printf(" P%d->fork%d", i, (i + 1) % N);
      printf("\n");
      exit(0);              /* the threads can never finish; end the process */
    }
    last_total = total;
  }
  for (int i = 0; i < N; i++)
    pthread_join(t[i], NULL);
  printf("%s: all %d philosophers ate %d times each\n", use_order ? "ordered" : "naive", N, MEALS);
}

int main(int argc, char **argv)
{
  if (argc > 1 && strcmp(argv[1], "naive") == 0)
    run(0);
  else
    run(1);
  return 0;
}
