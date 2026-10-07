/*
 * bug2_dead_arguments.c - Module 02 assignment, part B (2/3).
 *
 * Expected output (exactly):
 *     total: 56
 *
 * AddressSanitizer reports an error. Find the bug, explain it in a
 * comment, and fix it. Keep the split into start_workers() and
 * finish_workers() - real programs are structured like this all the time
 * (think: a server that starts workers in an init function).
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 8

struct job {
  int input;
  int output;
};

static pthread_t threads[N];
static struct job *jobs_seen_by_main[N];

static void *worker(void *arg)
{
  struct job *j = arg;
  usleep(20 * 1000);          /* pretend to do real work */
  j->output = j->input * 2;
  return NULL;
}

static void start_workers(void)
{
  struct job jobs[N];
  for (int i = 0; i < N; i++) {
    jobs[i].input = i;
    jobs[i].output = 0;
    jobs_seen_by_main[i] = &jobs[i];
    pthread_create(&threads[i], NULL, worker, &jobs[i]);
  }
}

static int finish_workers(void)
{
  int total = 0;
  for (int i = 0; i < N; i++) {
    pthread_join(threads[i], NULL);
    total += jobs_seen_by_main[i]->output;
  }
  return total;
}

int main(void)
{
  start_workers();
  printf("total: %d\n", finish_workers());
  return 0;
}
