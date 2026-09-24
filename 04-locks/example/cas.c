/*
 * cas.c - compare-and-swap (CAS), the universal atomic building block.
 *
 *   bool CAS(addr, &expected, desired):
 *       atomically { if (*addr == expected) { *addr = desired; return true; }
 *                    else { expected = *addr; return false; } }
 *
 * (x86: `lock cmpxchg`.) Any "read, compute, write back" update can be
 * made atomic with a CAS retry loop:
 *
 *     old = load(x);
 *     do { new = f(old); } while (!CAS(&x, &old, new));   // retry if someone changed x
 *
 * Below: an atomic maximum (no hardware "atomic max" instruction exists)
 * and a tiny lock built from CAS instead of test-and-set.
 */
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define THREADS 8
#define PER_THREAD 200000

static atomic_long max_seen = 0;

static void atomic_max(atomic_long *x, long v)
{
  long old = atomic_load(x);
  while (old < v && !atomic_compare_exchange_weak(x, &old, v)) {
    /* CAS failed: another thread changed *x. `old` now holds the fresh
     * value; the loop re-checks whether v is still larger. */
  }
}

/* A lock from CAS: 0 = free, 1 = taken. Equivalent to test-and-set. */
static atomic_int cas_lock_word = 0;
static long protected_counter = 0;

static void cas_lock(void)
{
  int expected = 0;
  while (!atomic_compare_exchange_weak(&cas_lock_word, &expected, 1))
    expected = 0;             /* CAS wrote the current value (1) into expected; reset */
}

static void cas_unlock(void) { atomic_store(&cas_lock_word, 0); }

static void *worker(void *arg)
{
  long id = (long)arg;
  unsigned s = (unsigned)id * 7919u + 1;
  for (int i = 0; i < PER_THREAD; i++) {
    s = s * 1103515245u + 12345u;
    atomic_max(&max_seen, (long)(s >> 8) % 1000000 + id * 1000000);
    cas_lock();
    protected_counter++;
    cas_unlock();
  }
  return NULL;
}

int main(void)
{
  pthread_t t[THREADS];
  for (long i = 0; i < THREADS; i++)
    pthread_create(&t[i], NULL, worker, (void *)i);
  for (int i = 0; i < THREADS; i++)
    pthread_join(t[i], NULL);
  printf("atomic max  = %ld (thread %d's range is the highest: 7000000..7999999)\n",
         atomic_load(&max_seen), THREADS - 1);
  printf("CAS lock    = %ld of %d increments\n", protected_counter, THREADS * PER_THREAD);
  return 0;
}
