/* Reference solution for Module 05 (header with the added fields). */
#pragma once
#include <pthread.h>

typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t nonzero;
  unsigned value;
} msem_t;

int msem_init(msem_t *s, unsigned value);
void msem_destroy(msem_t *s);
void msem_wait(msem_t *s);
int msem_trywait(msem_t *s);
void msem_post(msem_t *s);

typedef struct {
  pthread_mutex_t lock;
  pthread_cond_t readers_ok;    /* readers wait here                        */
  pthread_cond_t writer_ok;     /* writers wait here                        */
  int active_readers;           /* readers currently holding the lock       */
  int active_writer;            /* 1 while a writer holds the lock          */
  int waiting_writers;          /* writers blocked in rw_write_lock         */
} rwlock_t;

int rw_init(rwlock_t *rw);
void rw_destroy(rwlock_t *rw);
void rw_read_lock(rwlock_t *rw);
void rw_read_unlock(rwlock_t *rw);
void rw_write_lock(rwlock_t *rw);
void rw_write_unlock(rwlock_t *rw);

typedef struct {
  unsigned chairs;
  void (*cut_hair)(void *ctx);
  void *ctx;
  pthread_mutex_t lock;
  pthread_cond_t customer_ready;  /* barber sleeps here                     */
  pthread_cond_t haircut_done;    /* customers wait here for their haircut  */
  unsigned waiting;               /* customers sitting in waiting chairs    */
  unsigned long next_ticket;      /* ticket for the next customer who sits  */
  unsigned long finished;         /* tickets < finished have had their cut  */
  int closed;
} shop_t;

int shop_init(shop_t *s, unsigned chairs, void (*cut_hair)(void *), void *ctx);
void shop_destroy(shop_t *s);
int shop_visit(shop_t *s);
void barber_run(shop_t *s);
void shop_close(shop_t *s);
