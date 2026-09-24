/*
 * Reference solution for Module 05.
 */
#include "sync.h"

/* ------------------------------ Part A ------------------------------ */

int msem_init(msem_t *s, unsigned value)
{
  s->value = value;
  if (pthread_mutex_init(&s->lock, NULL) != 0)
    return -1;
  if (pthread_cond_init(&s->nonzero, NULL) != 0) {
    pthread_mutex_destroy(&s->lock);
    return -1;
  }
  return 0;
}

void msem_destroy(msem_t *s)
{
  pthread_cond_destroy(&s->nonzero);
  pthread_mutex_destroy(&s->lock);
}

void msem_wait(msem_t *s)
{
  pthread_mutex_lock(&s->lock);
  while (s->value == 0)                     /* while: another waiter may have */
    pthread_cond_wait(&s->nonzero, &s->lock);  /* taken the unit first        */
  s->value--;
  pthread_mutex_unlock(&s->lock);
}

int msem_trywait(msem_t *s)
{
  int ret = -1;
  pthread_mutex_lock(&s->lock);
  if (s->value > 0) {
    s->value--;
    ret = 0;
  }
  pthread_mutex_unlock(&s->lock);
  return ret;
}

void msem_post(msem_t *s)
{
  pthread_mutex_lock(&s->lock);
  s->value++;
  pthread_cond_signal(&s->nonzero);         /* one unit -> one waiter can proceed */
  pthread_mutex_unlock(&s->lock);
}

/* ------------------------------ Part B ------------------------------ */

int rw_init(rwlock_t *rw)
{
  pthread_mutex_init(&rw->lock, NULL);
  pthread_cond_init(&rw->readers_ok, NULL);
  pthread_cond_init(&rw->writer_ok, NULL);
  rw->active_readers = 0;
  rw->active_writer = 0;
  rw->waiting_writers = 0;
  return 0;
}

void rw_destroy(rwlock_t *rw)
{
  pthread_cond_destroy(&rw->writer_ok);
  pthread_cond_destroy(&rw->readers_ok);
  pthread_mutex_destroy(&rw->lock);
}

void rw_read_lock(rwlock_t *rw)
{
  pthread_mutex_lock(&rw->lock);
  /* Writer preference: also wait if a writer is merely WAITING. Without
   * "|| waiting_writers" a continuous stream of readers could keep
   * active_readers > 0 forever and starve every writer. */
  while (rw->active_writer || rw->waiting_writers > 0)
    pthread_cond_wait(&rw->readers_ok, &rw->lock);
  rw->active_readers++;
  pthread_mutex_unlock(&rw->lock);
}

void rw_read_unlock(rwlock_t *rw)
{
  pthread_mutex_lock(&rw->lock);
  if (--rw->active_readers == 0 && rw->waiting_writers > 0)
    pthread_cond_signal(&rw->writer_ok);    /* last reader out lets a writer in */
  pthread_mutex_unlock(&rw->lock);
}

void rw_write_lock(rwlock_t *rw)
{
  pthread_mutex_lock(&rw->lock);
  rw->waiting_writers++;                    /* from now on, new readers queue up */
  while (rw->active_writer || rw->active_readers > 0)
    pthread_cond_wait(&rw->writer_ok, &rw->lock);
  rw->waiting_writers--;
  rw->active_writer = 1;
  pthread_mutex_unlock(&rw->lock);
}

void rw_write_unlock(rwlock_t *rw)
{
  pthread_mutex_lock(&rw->lock);
  rw->active_writer = 0;
  if (rw->waiting_writers > 0)
    pthread_cond_signal(&rw->writer_ok);    /* one writer at a time */
  else
    pthread_cond_broadcast(&rw->readers_ok);  /* ALL waiting readers may enter */
  pthread_mutex_unlock(&rw->lock);
}

/* ------------------------------ Part C ------------------------------ */
/*
 * Tickets make "a customer returns only after HER haircut" easy: the
 * barber serves customers in ticket order (FIFO), `finished` counts
 * completed haircuts, so ticket t is done exactly when finished > t.
 * The customer being cut no longer occupies a waiting chair.
 *
 * The classic textbook solution uses three semaphores (customers,
 * barber_ready, mutex); it is shorter but makes "wait for MY haircut" and
 * a clean shutdown harder. Monitors (mutex + condvars) express both
 * directly.
 */

int shop_init(shop_t *s, unsigned chairs, void (*cut_hair)(void *), void *ctx)
{
  s->chairs = chairs;
  s->cut_hair = cut_hair;
  s->ctx = ctx;
  pthread_mutex_init(&s->lock, NULL);
  pthread_cond_init(&s->customer_ready, NULL);
  pthread_cond_init(&s->haircut_done, NULL);
  s->waiting = 0;
  s->next_ticket = 0;
  s->finished = 0;
  s->closed = 0;
  return 0;
}

void shop_destroy(shop_t *s)
{
  pthread_cond_destroy(&s->haircut_done);
  pthread_cond_destroy(&s->customer_ready);
  pthread_mutex_destroy(&s->lock);
}

int shop_visit(shop_t *s)
{
  pthread_mutex_lock(&s->lock);
  if (s->waiting >= s->chairs) {            /* all chairs taken: leave */
    pthread_mutex_unlock(&s->lock);
    return 0;
  }
  unsigned long my = s->next_ticket++;
  s->waiting++;
  pthread_cond_signal(&s->customer_ready);  /* wake the barber if he sleeps */
  while (s->finished <= my)
    pthread_cond_wait(&s->haircut_done, &s->lock);
  pthread_mutex_unlock(&s->lock);
  return 1;
}

void barber_run(shop_t *s)
{
  pthread_mutex_lock(&s->lock);
  for (;;) {
    while (s->waiting == 0 && !s->closed)
      pthread_cond_wait(&s->customer_ready, &s->lock);   /* sleep */
    if (s->waiting == 0)                     /* closed and nobody left */
      break;
    s->waiting--;                            /* next customer -> barber chair */
    pthread_mutex_unlock(&s->lock);
    s->cut_hair(s->ctx);                     /* never call out while holding the lock */
    pthread_mutex_lock(&s->lock);
    s->finished++;
    /* broadcast: several customers wait on this condvar, only the one whose
     * ticket just finished can proceed - signal might wake the wrong one. */
    pthread_cond_broadcast(&s->haircut_done);
  }
  pthread_mutex_unlock(&s->lock);
}

void shop_close(shop_t *s)
{
  pthread_mutex_lock(&s->lock);
  s->closed = 1;
  pthread_cond_signal(&s->customer_ready);  /* a sleeping barber must notice */
  pthread_mutex_unlock(&s->lock);
}
