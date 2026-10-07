/*
 * uthread.h - Module 10 assignment (capstone): a user-level thread library.
 *
 * Many threads on ONE kernel thread, switched by the library itself with
 * getcontext/makecontext/swapcontext. The interface copies pthreads, and
 * the problems are the ones you will solve in SWEB for P1:
 *   - a thread is born with a fresh stack and a start routine;
 *   - returning from the start routine must end the thread properly;
 *   - its return value waits in a "zombie" until somebody joins it;
 *   - a detached or joined thread's stack must be freed - but a thread
 *     cannot free the stack it is running on;
 *   - waiting (join, mutex, condition) means BLOCKING, not spinning;
 *   - errors: joining yourself, a thread that does not exist, a detached
 *     one, a cycle of joins.
 *
 * Scheduling: one FIFO ready queue. New threads, threads that yield and
 * threads that are woken up are appended at the END; the thread at the
 * FRONT runs next. Thread ids are never reused; the thread that called
 * into the library first (main) has id 0.
 *
 * All functions return 0 on success or an errno value, like pthreads.
 */
#pragma once
#include <stddef.h>

#define UTHREAD_MAX        64               /* threads alive at once, incl. main and zombies */
#define UTHREAD_STACK_SIZE (64 * 1024)      /* plus one guard page below it */
#define UTHREAD_DEADLOCK_EXIT 3             /* exit status when every thread is blocked */

typedef int uthread_t;

/* A FIFO of thread ids - ready queue, mutex and condition waiters. */
struct uthread_queue {
  uthread_t item[UTHREAD_MAX];
  int head, len;
};

/* ------------------------------ Part A ------------------------------ */

/* Start start(arg) in a new thread; its id goes to *thread. The new thread
 * is appended to the ready queue - the caller keeps running.
 * EAGAIN if UTHREAD_MAX threads exist (zombies count), EINVAL if thread
 * or start is NULL. */
int uthread_create(uthread_t *thread, void *(*start)(void *), void *arg);

uthread_t uthread_self(void);

/* Go to the end of the ready queue and let the front thread run (no-op
 * if nobody else is ready). */
void uthread_yield(void);

/* End the calling thread with retval (a return from the start routine
 * does the same). When the last thread ends, the process exits with 0.
 * If no thread can run any more but some are blocked, the library prints
 * "uthread: deadlock" to stderr and exits with UTHREAD_DEADLOCK_EXIT. */
_Noreturn void uthread_exit(void *retval);

/*
 * Wait until thread ends; its return value goes to *retval (if not NULL).
 * Afterwards the thread is gone: its stack is freed, its id invalid.
 *   ESRCH    no thread with that id (never existed or already joined)
 *   EDEADLK  thread is the caller, or thread is (directly or through a
 *            chain of joins) waiting to join the caller
 *   EINVAL   thread is detached, or another thread is already joining it
 */
int uthread_join(uthread_t thread, void **retval);

/*
 * Nobody will join thread: free it as soon as it ends (at once, if it
 * already has). ESRCH as for join; EINVAL if already detached or somebody
 * is joining it.
 */
int uthread_detach(uthread_t thread);

/* For the tests: how many thread stacks are currently allocated. */
int uthread_live_stacks(void);

/* ------------------------------ Part B ------------------------------ */

/* A blocking mutex. Unlocking with waiters hands the mutex directly to the
 * first waiter (FIFO, no barging: the unlocker cannot grab it back). */
typedef struct {
  uthread_t owner;                    /* -1: free */
  struct uthread_queue waiters;
} uthread_mutex_t;
#define UTHREAD_MUTEX_INITIALIZER {-1, {{0}, 0, 0}}

int uthread_mutex_lock(uthread_mutex_t *m);     /* EDEADLK if the caller owns it */
int uthread_mutex_trylock(uthread_mutex_t *m);  /* EBUSY if owned by anyone */
int uthread_mutex_unlock(uthread_mutex_t *m);   /* EPERM if the caller does not own it */

typedef struct {
  struct uthread_queue waiters;
} uthread_cond_t;
#define UTHREAD_COND_INITIALIZER {{{0}, 0, 0}}

/* Atomically unlock m and block; after a signal/broadcast, lock m again
 * before returning. EPERM if the caller does not own m. */
int uthread_cond_wait(uthread_cond_t *c, uthread_mutex_t *m);
int uthread_cond_signal(uthread_cond_t *c);     /* wake the longest waiter, if any */
int uthread_cond_broadcast(uthread_cond_t *c);  /* wake all waiters, in waiting order */

/* ------------------------- Part C (bonus): preemption ------------------------- */

/*
 * From now on, preempt the running thread every `usec` microseconds of CPU
 * time (0: stop), like the timer interrupt in SWEB. The library must then
 * protect its own data: a SIGVTALRM in the middle of uthread_* code is
 * exactly an interrupt in the middle of a kernel function.
 * Returns ENOSYS if not implemented.
 */
int uthread_preempt(long usec);
