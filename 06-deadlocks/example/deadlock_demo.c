/*
 * deadlock_demo.c - the smallest possible deadlock, made visible.
 *
 *   thread 1: lock(A); ...; lock(B)        thread 2: lock(B); ...; lock(A)
 *
 * If both take their first lock before either takes its second, each
 * waits for a lock the other one holds - forever. All four Coffman
 * conditions hold:
 *   1. mutual exclusion   - a mutex has at most one owner
 *   2. hold and wait      - each thread holds one lock while waiting
 *   3. no preemption      - nobody can take a lock away from its owner
 *   4. circular wait      - T1 waits for T2 waits for T1
 * Break any one and the deadlock is impossible. The usual choice: break 4
 * with a global lock order ("always A before B").
 *
 * To keep the demo from hanging, the second lock is taken with a timeout
 * (pthread_mutex_timedlock); a timeout means "we would have deadlocked".
 */
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t A = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t B = PTHREAD_MUTEX_INITIALIZER;
static pthread_barrier_t both_hold_first;

struct plan {
  const char *name;
  pthread_mutex_t *first, *second;
  const char *first_name, *second_name;
  int force;       /* wait until the other thread holds its first lock */
};

static int lock_with_timeout(pthread_mutex_t *m, int seconds)
{
  struct timespec until;
  clock_gettime(CLOCK_REALTIME, &until);
  until.tv_sec += seconds;
  return pthread_mutex_timedlock(m, &until);
}

static void *worker(void *arg)
{
  struct plan *p = arg;
  pthread_mutex_lock(p->first);
  printf("%s: holds %s, now wants %s\n", p->name, p->first_name, p->second_name);
  if (p->force)
    pthread_barrier_wait(&both_hold_first);   /* guarantee the bad interleaving */
  if (lock_with_timeout(p->second, 1) == ETIMEDOUT) {
    printf("%s: waited 1 s for %s -> DEADLOCK (giving up so the demo can end)\n",
           p->name, p->second_name);
    pthread_mutex_unlock(p->first);
    return NULL;
  }
  printf("%s: got both locks\n", p->name);
  pthread_mutex_unlock(p->second);
  pthread_mutex_unlock(p->first);
  return NULL;
}

static void run(const char *title, struct plan *p1, struct plan *p2)
{
  printf("=== %s ===\n", title);
  pthread_t t1, t2;
  pthread_create(&t1, NULL, worker, p1);
  pthread_create(&t2, NULL, worker, p2);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  printf("\n");
}

int main(void)
{
  pthread_barrier_init(&both_hold_first, NULL, 2);

  struct plan t1 = {"T1", &A, &B, "A", "B", 1};
  struct plan t2 = {"T2", &B, &A, "B", "A", 1};
  run("opposite order: A->B and B->A", &t1, &t2);

  /* Fix: both use the same global order A -> B. No barrier here: with a
   * common order one thread simply waits for the other to finish. */
  struct plan f1 = {"T1", &A, &B, "A", "B", 0};
  struct plan f2 = {"T2", &A, &B, "A", "B", 0};
  run("same order for everyone: A->B", &f1, &f2);

  pthread_barrier_destroy(&both_hold_first);
  return 0;
}
