/*
 * hello_threads.c - the complete life of a pthread in one file.
 *
 *   pthread_create(&tid, NULL, fn, arg)  start a new thread running fn(arg)
 *   pthread_join(tid, &ret)               wait for it, receive fn's return value
 *
 * A thread is "a second program counter + a second stack" inside the SAME
 * process: same PID, same address space, same open files. The kernel
 * schedules threads, not processes (see the TID printed below).
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 4

/* Everything a worker needs goes into one struct, and each thread gets
 * its OWN struct. Sharing one struct between all threads (or passing &i)
 * is the #1 beginner bug - see ../assignment/bugs/. */
struct worker_args {
  int id;
  int from, to;    /* sum the numbers in [from, to) */
  long result;     /* output: written by the worker, read by main after join */
};

static void *worker(void *p)
{
  struct worker_args *a = p;
  long sum = 0;
  for (int i = a->from; i < a->to; i++)
    sum += i;
  a->result = sum;
  /* gettid() = kernel thread id: differs per thread. getpid(): same for all. */
  printf("worker %d: pid %d, tid %d, summed [%d, %d) = %ld\n",
         a->id, getpid(), gettid(), a->from, a->to, sum);
  return NULL;           /* returning from fn == calling pthread_exit(NULL) */
}

int main(void)
{
  pthread_t tids[N];
  struct worker_args args[N];   /* lives until the end of main: long enough */

  printf("main    : pid %d, tid %d\n", getpid(), gettid());
  for (int i = 0; i < N; i++) {
    args[i] = (struct worker_args){.id = i, .from = i * 250, .to = (i + 1) * 250};
    int err = pthread_create(&tids[i], NULL, worker, &args[i]);
    if (err != 0) {                   /* pthread functions return the error */
      fprintf(stderr, "pthread_create failed: %d\n", err);   /* code, not -1 */
      return 1;
    }
  }

  /* Join = wait for termination. Only after join is it safe to read
   * args[i].result: join makes the thread's writes visible to us. */
  long total = 0;
  for (int i = 0; i < N; i++) {
    pthread_join(tids[i], NULL);
    total += args[i].result;
  }
  printf("main    : total = %ld (expected %d)\n", total, 999 * 1000 / 2);
  return 0;
}
