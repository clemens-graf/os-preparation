/*
 * sync.c - Module 05 assignment. Replace every TODO.
 * Build & test:  make test   (or make test-sem / test-rw / test-barber)
 *
 * The rules from chapter 5, one more time:
 *   - shared state only under the mutex;
 *   - wait in a while loop: while (!condition) pthread_cond_wait(&cv, &m);
 *   - change the state, then signal/broadcast, while holding the mutex;
 *   - broadcast when more than one waiter may be able to proceed, or when
 *     waiters wait for DIFFERENT conditions on the same condvar.
 */
#include "sync.h"

/* ------------------------------ Part A ------------------------------ */

int msem_init(msem_t *s, unsigned value)
{
  /* TODO */
  return -1;
}

void msem_destroy(msem_t *s)
{
  /* TODO */
}

void msem_wait(msem_t *s)
{
  /* TODO */
}

int msem_trywait(msem_t *s)
{
  /* TODO */
  return -1;
}

void msem_post(msem_t *s)
{
  /* TODO */
}

/* ------------------------------ Part B ------------------------------ */

int rw_init(rwlock_t *rw)
{
  /* TODO */
  return -1;
}

void rw_destroy(rwlock_t *rw)
{
  /* TODO */
}

void rw_read_lock(rwlock_t *rw)
{
  /* TODO: wait while a writer is active OR waiting (writer preference) */
}

void rw_read_unlock(rwlock_t *rw)
{
  /* TODO */
}

void rw_write_lock(rwlock_t *rw)
{
  /* TODO: announce yourself as waiting, then wait for exclusive access */
}

void rw_write_unlock(rwlock_t *rw)
{
  /* TODO: who should be woken - the next writer, or all readers? */
}

/* ------------------------------ Part C ------------------------------ */

int shop_init(shop_t *s, unsigned chairs, void (*cut_hair)(void *), void *ctx)
{
  s->chairs = chairs;
  s->cut_hair = cut_hair;
  s->ctx = ctx;
  /* TODO */
  return -1;
}

void shop_destroy(shop_t *s)
{
  /* TODO */
}

int shop_visit(shop_t *s)
{
  /* TODO */
  return 0;
}

void barber_run(shop_t *s)
{
  /* TODO */
}

void shop_close(shop_t *s)
{
  /* TODO */
}
