/*
 * lost_wakeup.c - why "check the condition, then go to sleep" must be
 * one atomic step.
 *
 * A condition variable has no memory: pthread_cond_signal wakes a thread
 * that is waiting RIGHT NOW, and does nothing if nobody waits yet.
 * If the waiter checks the condition without holding the mutex, the
 * signal can fall between its check and its wait:
 *
 *     waiter:                            waker:
 *     if (!ready)              (1)
 *                                        ready = 1;             (2)
 *                                        signal(cv);            (3) nobody waits: LOST
 *     wait(cv)                 (4)       ... sleeps forever
 *
 * Correct: the waiter holds the mutex from the check until wait() has
 * registered it as a sleeper (wait releases the mutex atomically), and the
 * waker changes `ready` under the same mutex. Then (2)+(3) cannot happen
 * between (1) and (4).
 *
 * SWEB's Scheduler::wake() contains
 *     while (thread_to_wake->getState() != Sleeping) yield();
 * i.e. "wait until the other thread has really gone to sleep, then wake
 * it" - a defence against exactly this race.
 */
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static atomic_int ready;              /* atomic only to keep TSan quiet; the bug is logical */

/* ------------------------- broken waiter ------------------------- */
static void *broken_waiter(void *arg)
{
  (void)arg;
  if (!atomic_load(&ready)) {         /* (1) check WITHOUT the mutex */
    usleep(200 * 1000);               /* widen the window on purpose */
    pthread_mutex_lock(&m);
    struct timespec until;
    clock_gettime(CLOCK_REALTIME, &until);
    until.tv_sec += 2;                /* safety net so the demo ends */
    int rc = pthread_cond_timedwait(&cv, &m, &until);   /* (4) */
    pthread_mutex_unlock(&m);
    if (rc == ETIMEDOUT) {
      printf("broken waiter: still asleep after 2 s -> the wake-up was LOST\n");
      return NULL;
    }
  }
  printf("broken waiter: woke up\n");
  return NULL;
}

/* ------------------------- correct waiter ------------------------ */
static int ready_locked;              /* protected by m */

static void *correct_waiter(void *arg)
{
  (void)arg;
  pthread_mutex_lock(&m);
  while (!ready_locked) {             /* check under the mutex ...           */
    usleep(200 * 1000);               /* same delay: harmless, the waker     */
    pthread_cond_wait(&cv, &m);       /* cannot change ready_locked while we */
  }                                   /* hold m                              */
  pthread_mutex_unlock(&m);
  printf("correct waiter: woke up\n");
  return NULL;
}

int main(void)
{
  pthread_t t;

  printf("--- check without the mutex ---\n");
  pthread_create(&t, NULL, broken_waiter, NULL);
  usleep(50 * 1000);                  /* the waiter has checked, not yet waited */
  atomic_store(&ready, 1);            /* (2) */
  pthread_mutex_lock(&m);
  pthread_cond_signal(&cv);           /* (3) nobody is waiting yet */
  pthread_mutex_unlock(&m);
  printf("waker: signalled\n");
  pthread_join(t, NULL);

  printf("\n--- check under the mutex ---\n");
  pthread_create(&t, NULL, correct_waiter, NULL);
  usleep(50 * 1000);
  pthread_mutex_lock(&m);             /* blocks until the waiter really waits */
  ready_locked = 1;
  pthread_cond_signal(&cv);
  pthread_mutex_unlock(&m);
  printf("waker: signalled\n");
  pthread_join(t, NULL);
  return 0;
}
