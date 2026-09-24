/*
 * locks.h - Module 04 assignment: build your own locks from atomics.
 * Implement everything in locks.c. Do NOT rename the struct fields - the
 * tests look at them. You may not use pthread_mutex / pthread_spin here.
 *
 * Use C11 <stdatomic.h>: atomic_exchange, atomic_compare_exchange_strong,
 * atomic_fetch_add, atomic_load, atomic_store. The default (seq_cst)
 * memory order is always correct; weaker orders are an optional
 * optimisation (acquire when taking a lock, release when giving it back).
 */
#pragma once
#include <stdatomic.h>

/* ------------------------------------------------------------------ */
/* Part A: spinlock (like SWEB's SpinLock and pthread_spin_*)          */
/* ------------------------------------------------------------------ */
typedef struct {
  atomic_int locked;      /* 0 = free, 1 = taken */
} my_spinlock_t;

void spin_init(my_spinlock_t *l);

/* Test-and-test-and-set: while the lock looks taken, only READ it
 * (cheap, stays in your cache); only when it looks free, try the atomic
 * exchange. After SPIN_TRIES unsuccessful looks, call sched_yield() so a
 * preempted lock holder can run (SWEB's SpinLock always yields). */
#define SPIN_TRIES 100
void spin_lock(my_spinlock_t *l);

/* 0 if the lock was acquired, EBUSY if it is taken. Never waits. */
int spin_trylock(my_spinlock_t *l);

void spin_unlock(my_spinlock_t *l);

/* ------------------------------------------------------------------ */
/* Part B: ticket lock - a FAIR spinlock                               */
/* ------------------------------------------------------------------ */
/* Like the ticket machine at a counter: take a number (fetch-and-add on
 * next_ticket), wait until now_serving shows your number. Threads get
 * the lock in exactly the order they arrived (FIFO) - a plain spinlock
 * gives no such guarantee and can starve a thread forever. */
typedef struct {
  atomic_uint next_ticket;
  atomic_uint now_serving;
} ticketlock_t;

void ticket_init(ticketlock_t *l);
void ticket_lock(ticketlock_t *l);
void ticket_unlock(ticketlock_t *l);

/* ------------------------------------------------------------------ */
/* Part C (BONUS): a mutex whose waiters SLEEP instead of spinning      */
/* ------------------------------------------------------------------ */
/* state: 0 = unlocked, 1 = locked with no waiters, 2 = locked, maybe
 * waiters. Waiters block in the kernel with the futex syscall:
 *   futex_wait(&state, 2)  sleeps only if state is still 2 (checked
 *                          atomically by the kernel - no lost wake-up)
 *   futex_wake(&state, 1)  wakes one sleeper
 * Helpers are provided in locks.c. Read "Futexes Are Tricky" (U. Drepper),
 * section "mutex2", before you start.
 * Leave both functions returning ENOSYS to skip the bonus tests. */
typedef struct {
  atomic_int state;
} fmutex_t;

#define FMUTEX_INIT {0}
int fmutex_lock(fmutex_t *m);     /* 0 on success */
int fmutex_unlock(fmutex_t *m);   /* 0 on success */
