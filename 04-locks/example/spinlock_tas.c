/*
 * spinlock_tas.c - the simplest real lock: test-and-set.
 *
 * test_and_set(&x) atomically does { old = x; x = 1; return old; } as ONE
 * indivisible hardware operation (x86: `xchg`). If it returns 0, the lock
 * was free and we now own it; if it returns 1, someone else owns it.
 *
 * SWEB's SpinLock does exactly this (ArchThreads::testSetLock is
 * __sync_lock_test_and_set) - with one twist: instead of burning the CPU
 * in the loop it calls Scheduler::yield(). On a single CPU, spinning is
 * pointless: the lock holder cannot run (and release the lock) while you
 * spin on the only core.
 *
 * This program runs 4x more threads than you have cores (so lock holders
 * really get preempted) and compares:
 *   (a) pure spinning       (b) spin + sched_yield()     (c) pthread_mutex (sleeps)
 */
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define ITER 20000

static atomic_flag lock = ATOMIC_FLAG_INIT;
static long counter;
static int mode;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void spin_lock(void)
{
  /* atomic_flag_test_and_set: set the flag, return its previous value.
   * The default memory order is seq_cst, so everything the previous owner
   * wrote before unlocking is visible to us after we get the lock. */
  while (atomic_flag_test_and_set(&lock)) {
    if (mode == 1)
      sched_yield();        /* give the lock holder a chance to run */
  }
}

static void spin_unlock(void)
{
  atomic_flag_clear(&lock); /* release: our writes become visible to the next owner */
}

static void *worker(void *arg)
{
  (void)arg;
  for (int i = 0; i < ITER; i++) {
    if (mode == 2) {
      pthread_mutex_lock(&mutex);
      counter++;
      pthread_mutex_unlock(&mutex);
    } else {
      spin_lock();
      counter++;            /* protected: only the lock owner gets here */
      spin_unlock();
    }
  }
  return NULL;
}

static void run(int m, int nthreads)
{
  static const char *names[] = {"pure spinning", "spin + sched_yield", "pthread_mutex"};
  mode = m;
  counter = 0;
  pthread_t t[256];
  struct timespec a, b, ca, cb;
  clock_gettime(CLOCK_MONOTONIC, &a);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ca);
  for (int i = 0; i < nthreads; i++)
    pthread_create(&t[i], NULL, worker, NULL);
  for (int i = 0; i < nthreads; i++)
    pthread_join(t[i], NULL);
  clock_gettime(CLOCK_MONOTONIC, &b);
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cb);
  double wall = (double)(b.tv_sec - a.tv_sec) * 1e3 + (double)(b.tv_nsec - a.tv_nsec) / 1e6;
  double cpu = (double)(cb.tv_sec - ca.tv_sec) * 1e3 + (double)(cb.tv_nsec - ca.tv_nsec) / 1e6;
  printf("(%c) %-20s counter=%ld  wall %8.1f ms  cpu %9.1f ms\n",
         'a' + m, names[m], counter, wall, cpu);
}

int main(void)
{
  long cores = sysconf(_SC_NPROCESSORS_ONLN);
  int nthreads = (int)(cores * 4 > 256 ? 256 : cores * 4);
  printf("%d threads on %ld cores, %d lock/unlock each\n", nthreads, cores, ITER);
  run(0, nthreads);
  run(1, nthreads);
  run(2, nthreads);
  printf("cpu >> wall means: many cores were busy doing nothing useful (spinning).\n");
  return 0;
}
