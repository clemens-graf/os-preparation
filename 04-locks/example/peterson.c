/*
 * peterson.c - mutual exclusion with nothing but loads and stores.
 *
 * Peterson's algorithm (1981) for two threads, i in {0, 1}:
 *
 *   lock(i):   flag[i] = 1;            // "I want to enter"
 *              turn = 1 - i;           // "but you go first if you want too"
 *              while (flag[1-i] && turn == 1-i)
 *                ;                     // wait while the other wants in and has priority
 *   unlock(i): flag[i] = 0;
 *
 * On paper it is correct. On a modern CPU the plain version FAILS: x86
 * may let a later LOAD (of flag[1-i]) overtake an earlier STORE (to
 * flag[i]) that still sits in the core's store buffer. Then both threads
 * read "the other one does not want in" and both enter.
 *
 * The fix is a memory fence / sequentially consistent atomics, which
 * forbid that reordering. This is why real locks are built from atomic
 * read-modify-write instructions (xchg, cmpxchg, lock xadd) that include
 * the fence - see spinlock_tas.c.
 */
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define ITER 5000000

/* ---------------- version 1: volatile loads/stores only ---------------- */
static volatile int flag_v[2];
static volatile int turn_v;

static void lock_plain(int i)
{
  flag_v[i] = 1;
  turn_v = 1 - i;
  while (flag_v[1 - i] && turn_v == 1 - i)
    ;
}

static void unlock_plain(int i) { flag_v[i] = 0; }

/* ---------------- version 2: sequentially consistent atomics ---------------- */
static atomic_int flag_a[2];
static atomic_int turn_a;

static void lock_seqcst(int i)
{
  atomic_store(&flag_a[i], 1);       /* seq_cst store: acts as a full fence here */
  atomic_store(&turn_a, 1 - i);
  while (atomic_load(&flag_a[1 - i]) && atomic_load(&turn_a) == 1 - i)
    ;
}

static void unlock_seqcst(int i) { atomic_store(&flag_a[i], 0); }

/* ---------------- the critical section ---------------- */
static volatile long counter;
static atomic_int inside;             /* how many threads are in the CS right now */
static atomic_long violations;

static int use_atomics;

static void *worker(void *arg)
{
  int i = (int)(long)arg;
  for (long n = 0; n < ITER; n++) {
    if (use_atomics) lock_seqcst(i); else lock_plain(i);

    if (atomic_fetch_add(&inside, 1) != 0)     /* someone else is in here too! */
      atomic_fetch_add(&violations, 1);
    counter++;
    atomic_fetch_sub(&inside, 1);

    if (use_atomics) unlock_seqcst(i); else unlock_plain(i);
  }
  return NULL;
}

static void run(int atomics)
{
  use_atomics = atomics;
  counter = 0;
  atomic_store(&violations, 0);
  pthread_t a, b;
  pthread_create(&a, NULL, worker, (void *)0L);
  pthread_create(&b, NULL, worker, (void *)1L);
  pthread_join(a, NULL);
  pthread_join(b, NULL);
  printf("%-26s counter = %ld of %d, both inside the CS %ld time(s)\n",
         atomics ? "seq_cst atomics:" : "plain volatile variables:",
         counter, 2 * ITER, atomic_load(&violations));
}

int main(void)
{
  run(0);
  run(1);
  return 0;
}
