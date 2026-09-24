/*
 * sync.h - Module 05 assignment: build synchronisation primitives from a
 * mutex and condition variables, then solve two classic problems with them.
 *
 * This is (almost literally) P1's elective task in SWEB: "user-space
 * synchronisation: mutexes, condition variables and semaphores ... Demo
 * with Readers-Writers and Sleeping Barber."
 *
 * Allowed: pthread_mutex_*, pthread_cond_*. Not allowed: sem_*,
 * pthread_rwlock_*, pthread_barrier_* (you are building those).
 * You may add fields to every struct.
 */
#pragma once
#include <pthread.h>

/* ------------------------------------------------------------------ */
/* Part A: counting semaphore                                          */
/* ------------------------------------------------------------------ */
typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t nonzero;
  unsigned value;
} msem_t;

int msem_init(msem_t *s, unsigned value);   /* 0 on success */
void msem_destroy(msem_t *s);
void msem_wait(msem_t *s);                   /* P / down: blocks while value == 0 */
int msem_trywait(msem_t *s);                 /* 0 if decremented, -1 if value was 0 */
void msem_post(msem_t *s);                   /* V / up */

/* ------------------------------------------------------------------ */
/* Part B: readers-writer lock, writer-preferring                      */
/* ------------------------------------------------------------------ */
/* Any number of readers may hold the lock at the same time, OR exactly
 * one writer. Writer preference: as soon as a writer is WAITING, new
 * readers must wait too - otherwise a steady stream of readers starves
 * the writer forever. */
typedef struct {
  pthread_mutex_t lock;
  /* TODO: add condition variables and counters */
} rwlock_t;

int rw_init(rwlock_t *rw);
void rw_destroy(rwlock_t *rw);
void rw_read_lock(rwlock_t *rw);
void rw_read_unlock(rwlock_t *rw);
void rw_write_lock(rwlock_t *rw);
void rw_write_unlock(rwlock_t *rw);

/* ------------------------------------------------------------------ */
/* Part C: the sleeping barber                                         */
/* ------------------------------------------------------------------ */
/* A barber shop with one barber, one barber chair and `chairs` waiting
 * chairs.
 *   - A customer arrives: if a waiting chair is free, she sits down and
 *     waits for her haircut; if all waiting chairs are taken she leaves.
 *   - The barber sleeps while nobody is waiting; otherwise he takes the
 *     next waiting customer and cuts her hair (call cut_hair()).
 *   - A customer returns from shop_visit only after HER haircut is done.
 *   - shop_close(): no more customers will come. The barber finishes the
 *     customers that are already waiting, then barber_run returns.
 * Use your msem_t (and/or mutex + condvars). */
typedef struct {
  unsigned chairs;
  void (*cut_hair)(void *ctx);  /* called by the barber once per haircut */
  void *ctx;
  /* TODO: your state */
} shop_t;

int shop_init(shop_t *s, unsigned chairs, void (*cut_hair)(void *), void *ctx);
void shop_destroy(shop_t *s);

/* Customer: returns 1 after getting a haircut, 0 if the shop was full. */
int shop_visit(shop_t *s);

/* Barber thread body: serve customers until the shop is closed and empty. */
void barber_run(shop_t *s);

/* No more customers will arrive. */
void shop_close(shop_t *s);
