/*
 * locks.c - Module 04 assignment. Replace every TODO.
 * Build & test:  make test   (runs under ThreadSanitizer and optimised)
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
  /* TODO: test-and-test-and-set with sched_yield() after SPIN_TRIES
   * unsuccessful looks. */
}

int spin_trylock(my_spinlock_t *l)
{
  /* TODO */
  return EBUSY;
}

void spin_unlock(my_spinlock_t *l)
{
  /* TODO */
}

/* ------------------------------ Part B ------------------------------ */

void ticket_init(ticketlock_t *l)
{
  atomic_init(&l->next_ticket, 0);
  atomic_init(&l->now_serving, 0);
}

void ticket_lock(ticketlock_t *l)
{
  /* TODO: take a ticket, wait (yielding) until it is being served. */
}

void ticket_unlock(ticketlock_t *l)
{
  /* TODO */
}

/* --------------------------- Part C (bonus) --------------------------- */

/* Sleep until woken, but only if *addr still equals `expected`. */
static inline void futex_wait(atomic_int *addr, int expected)
{
  syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, expected, NULL, NULL, 0);
}

/* Wake up at most n threads sleeping on addr. */
static inline void futex_wake(atomic_int *addr, int n)
{
  syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, n, NULL, NULL, 0);
}

int fmutex_lock(fmutex_t *m)
{
  return ENOSYS;          /* TODO (bonus) */
}

int fmutex_unlock(fmutex_t *m)
{
  return ENOSYS;          /* TODO (bonus) */
}
