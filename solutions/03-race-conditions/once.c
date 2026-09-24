/*
 * Reference solution for Module 03, part C, including the bonus.
 *
 * The broken version is a check-then-act race: several threads can see
 * done == 0 and all run init. Holding a mutex across "check done, run
 * init, set done" makes that one indivisible step. Threads that arrive
 * while init is running block on the mutex - so nobody returns before init
 * has completed (the second guarantee).
 *
 * Bonus - the fast path: once done is set, taking the mutex on every call
 * is wasted work. We check `done` first WITHOUT the lock (double-checked
 * locking). This is only correct because
 *   - done is atomic (no data race on the flag itself), and
 *   - it is written with release and read with acquire ordering: a thread
 *     that sees done == 1 is guaranteed to also see everything init wrote.
 *     With a plain int the compiler/CPU could let it see done == 1 while
 *     init's writes are not visible yet.
 */
#include "once.h"

void my_once(my_once_t *o, void (*init)(void))
{
  if (atomic_load_explicit(&o->done, memory_order_acquire))
    return;                                   /* fast path, no lock */

  pthread_mutex_lock(&o->lock);
  if (!atomic_load_explicit(&o->done, memory_order_relaxed)) {  /* re-check under the lock */
    init();
    atomic_store_explicit(&o->done, 1, memory_order_release);
  }
  pthread_mutex_unlock(&o->lock);
}
