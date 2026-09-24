/*
 * Reference solution for Module 04, including the futex bonus.
 */
#include "locks.h"

#include <errno.h>
#include <limits.h>
#include <linux/futex.h>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>

/* ------------------------------ Part A ------------------------------ */

void spin_init(my_spinlock_t *l)
{
  atomic_init(&l->locked, 0);
}

void spin_lock(my_spinlock_t *l)
{
  int tries = 0;
  for (;;) {
    /* "test": plain loads while the lock is taken. They hit our own cache
     * line copy and generate no bus traffic, unlike repeated exchanges. */
    while (atomic_load_explicit(&l->locked, memory_order_relaxed)) {
      if (++tries >= SPIN_TRIES) {
        sched_yield();       /* the holder may be preempted: let it run */
        tries = 0;
      }
    }
    /* "test-and-set": looks free - try to take it atomically. Acquire:
     * everything the previous owner wrote before its release is visible. */
    if (atomic_exchange_explicit(&l->locked, 1, memory_order_acquire) == 0)
      return;
  }
}

int spin_trylock(my_spinlock_t *l)
{
  return atomic_exchange_explicit(&l->locked, 1, memory_order_acquire) == 0 ? 0 : EBUSY;
}

void spin_unlock(my_spinlock_t *l)
{
  /* Release: our writes in the critical section happen-before the next
   * owner's acquire. A plain store is enough - only the owner unlocks. */
  atomic_store_explicit(&l->locked, 0, memory_order_release);
}

/* ------------------------------ Part B ------------------------------ */

void ticket_init(ticketlock_t *l)
{
  atomic_init(&l->next_ticket, 0);
  atomic_init(&l->now_serving, 0);
}

void ticket_lock(ticketlock_t *l)
{
  /* fetch_add hands out every number exactly once, even under contention:
   * that is what makes the order FIFO. */
  unsigned my = atomic_fetch_add_explicit(&l->next_ticket, 1, memory_order_relaxed);
  int tries = 0;
  while (atomic_load_explicit(&l->now_serving, memory_order_acquire) != my) {
    if (++tries >= SPIN_TRIES) {
      sched_yield();
      tries = 0;
    }
  }
}

void ticket_unlock(ticketlock_t *l)
{
  /* Only the owner writes now_serving, so load + store is not a race.
   * Unsigned overflow wraps around consistently for both counters. */
  unsigned next = atomic_load_explicit(&l->now_serving, memory_order_relaxed) + 1;
  atomic_store_explicit(&l->now_serving, next, memory_order_release);
}

/* --------------------------- Part C (bonus) --------------------------- */

static inline void futex_wait(atomic_int *addr, int expected)
{
  syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, expected, NULL, NULL, 0);
}

static inline void futex_wake(atomic_int *addr, int n)
{
  syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, n, NULL, NULL, 0);
}

/*
 * Drepper's "mutex2". state: 0 free, 1 locked, 2 locked + maybe waiters.
 *
 * Fast path (no contention): one CAS 0 -> 1, no syscall at all.
 * Slow path: announce "there are waiters" by setting 2, then sleep in the
 * kernel. futex_wait re-checks *inside the kernel* that state is still 2
 * before sleeping: if the owner unlocked in between, the wait returns at
 * once. That kernel-side check is what prevents the lost wake-up that a
 * naive "check, then sleep" would have (module 05).
 */
int fmutex_lock(fmutex_t *m)
{
  int c = 0;
  if (atomic_compare_exchange_strong(&m->state, &c, 1))
    return 0;                                  /* was 0, now 1: ours */
  if (c != 2)
    c = atomic_exchange(&m->state, 2);         /* mark contended */
  while (c != 0) {                             /* 0 means we just took it (as 2) */
    futex_wait(&m->state, 2);
    c = atomic_exchange(&m->state, 2);
  }
  return 0;
}

int fmutex_unlock(fmutex_t *m)
{
  /* 1 -> 0: nobody waits, done. 2 -> 1: there may be sleepers - fully
   * release and wake one. It takes the lock as state 2 (it cannot know
   * whether other sleepers remain), so the next unlock wakes again. */
  if (atomic_fetch_sub(&m->state, 1) != 1) {
    atomic_store(&m->state, 0);
    futex_wake(&m->state, 1);
  }
  return 0;
}
