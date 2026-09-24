#include "check.h"
#include "locks.h"

#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>

/* ---- count sched_yield calls made by locks.c (ld --wrap) ---- */
static atomic_long yields;
int __real_sched_yield(void);
int __wrap_sched_yield(void)
{
  atomic_fetch_add(&yields, 1);
  return __real_sched_yield();
}

enum kind { SPIN, TICKET, FUTEX };
static const char *kind_name[] = {"spinlock", "ticket lock", "futex mutex"};

static my_spinlock_t sl;
static ticketlock_t tl;
static fmutex_t fm = FMUTEX_INIT;

static void lock(enum kind k)
{
  if (k == SPIN) spin_lock(&sl);
  else if (k == TICKET) ticket_lock(&tl);
  else fmutex_lock(&fm);
}

static void unlock(enum kind k)
{
  if (k == SPIN) spin_unlock(&sl);
  else if (k == TICKET) ticket_unlock(&tl);
  else fmutex_unlock(&fm);
}

/* ---------------- mutual exclusion ---------------- */
#ifndef THREADS
#define THREADS 8
#endif
#ifndef ITER
#define ITER 20000
#endif

static long counter;                 /* plain variable, protected by the lock under test */
static atomic_int inside;
static atomic_long violations;

static void *hammer(void *arg)
{
  enum kind k = (enum kind)(intptr_t)arg;
  for (int i = 0; i < ITER; i++) {
    lock(k);
    if (atomic_fetch_add(&inside, 1) != 0)
      atomic_fetch_add(&violations, 1);
    counter++;
    atomic_fetch_sub(&inside, 1);
    unlock(k);
  }
  return NULL;
}

static void *mutual_exclusion(void *arg)
{
  enum kind k = (enum kind)(intptr_t)arg;
  counter = 0;
  atomic_store(&violations, 0);
  pthread_t t[THREADS];
  for (int i = 0; i < THREADS; i++)
    pthread_create(&t[i], NULL, hammer, arg);
  for (int i = 0; i < THREADS; i++)
    pthread_join(t[i], NULL);
  CHECK(atomic_load(&violations) == 0, "%s: two threads were inside the critical section %ld time(s)",
        kind_name[k], atomic_load(&violations));
  CHECK(counter == (long)THREADS * ITER, "%s: counter = %ld, expected %ld",
        kind_name[k], counter, (long)THREADS * ITER);
  return NULL;
}

static void test_spin_mutual_exclusion(void)
{
  spin_init(&sl);
  must_finish_within(60, "spinlock mutual exclusion", mutual_exclusion, (void *)(intptr_t)SPIN);
}

static void test_ticket_mutual_exclusion(void)
{
  ticket_init(&tl);
  must_finish_within(60, "ticket lock mutual exclusion", mutual_exclusion, (void *)(intptr_t)TICKET);
}

/* ---------------- spinlock details ---------------- */
static void test_spin_trylock(void)
{
  spin_init(&sl);
  CHECK_EQ(spin_trylock(&sl), 0);
  CHECK_EQ(spin_trylock(&sl), EBUSY);
  spin_unlock(&sl);
  CHECK_EQ(spin_trylock(&sl), 0);
  spin_unlock(&sl);
  spin_lock(&sl);
  CHECK_EQ(spin_trylock(&sl), EBUSY);
  spin_unlock(&sl);
}

static atomic_int waiter_got_lock;

static void *spin_waiter(void *arg)
{
  (void)arg;
  spin_lock(&sl);
  atomic_store(&waiter_got_lock, 1);
  spin_unlock(&sl);
  return NULL;
}

static void test_spin_waiter_yields(void)
{
  spin_init(&sl);
  atomic_store(&waiter_got_lock, 0);
  spin_lock(&sl);                      /* main holds the lock ... */
  atomic_store(&yields, 0);
  pthread_t t;
  pthread_create(&t, NULL, spin_waiter, NULL);
  sleep_ms(100);                       /* ... while a waiter tries to get it */
  CHECK(!atomic_load(&waiter_got_lock), "the waiter got the lock although main was holding it");
  CHECK(atomic_load(&yields) > 0, "a waiting thread never called sched_yield() in 100 ms");
  spin_unlock(&sl);
  pthread_join(t, NULL);
  CHECK(atomic_load(&waiter_got_lock), "the waiter did not get the lock after main released it");
}

/* ---------------- ticket lock fairness ---------------- */
static atomic_int order_idx;
static int order[3];

static void *ticket_waiter(void *arg)
{
  ticket_lock(&tl);
  order[atomic_fetch_add(&order_idx, 1)] = (int)(intptr_t)arg;
  ticket_unlock(&tl);
  return NULL;
}

static void test_ticket_fifo(void)
{
  ticket_init(&tl);
  atomic_store(&order_idx, 0);
  ticket_lock(&tl);                    /* main holds ticket 0 */
  pthread_t t[3];
  for (intptr_t id = 0; id < 3; id++) {
    pthread_create(&t[id], NULL, ticket_waiter, (void *)id);
    /* wait until thread `id` has drawn its ticket before starting the next */
    long long until = now_ms() + 2000;
    while (atomic_load(&tl.next_ticket) != (unsigned)id + 2 && now_ms() < until)
      sleep_ms(1);
    CHECK(atomic_load(&tl.next_ticket) == (unsigned)id + 2,
          "thread %ld did not take a ticket (next_ticket = %u)", (long)id, atomic_load(&tl.next_ticket));
  }
  ticket_unlock(&tl);
  for (int i = 0; i < 3; i++)
    pthread_join(t[i], NULL);
  CHECK(order[0] == 0 && order[1] == 1 && order[2] == 2,
        "threads got the lock in order %d %d %d, expected 0 1 2 (FIFO)", order[0], order[1], order[2]);
}

/* ---------------- bonus: futex mutex ---------------- */
static int fmutex_implemented(void)
{
  if (fmutex_lock(&fm) == ENOSYS)
    return 0;
  fmutex_unlock(&fm);
  return 1;
}

static void test_fmutex_mutual_exclusion(void)
{
  if (!fmutex_implemented())
    SKIP("bonus fmutex not implemented");
  must_finish_within(60, "futex mutex mutual exclusion", mutual_exclusion, (void *)(intptr_t)FUTEX);
}

static void *fmutex_waiter(void *arg)
{
  (void)arg;
  fmutex_lock(&fm);
  fmutex_unlock(&fm);
  return NULL;
}

static double cpu_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
  return (double)ts.tv_sec * 1e3 + (double)ts.tv_nsec / 1e6;
}

static void test_fmutex_waiters_sleep(void)
{
  if (!fmutex_implemented())
    SKIP("bonus fmutex not implemented");
  fmutex_lock(&fm);
  pthread_t t[4];
  for (int i = 0; i < 4; i++)
    pthread_create(&t[i], NULL, fmutex_waiter, NULL);
  sleep_ms(50);                        /* let them reach the lock */
  double before = cpu_ms();
  sleep_ms(300);
  double used = cpu_ms() - before;
  fmutex_unlock(&fm);
  for (int i = 0; i < 4; i++)
    pthread_join(t[i], NULL);
  CHECK(used < 100, "4 blocked waiters used %.0f ms of CPU in 300 ms - they spin instead of sleeping", used);
}

int main(void)
{
  RUN_TEST(test_spin_trylock);
  RUN_TEST(test_spin_mutual_exclusion);
  RUN_TEST(test_spin_waiter_yields);
  RUN_TEST(test_ticket_mutual_exclusion);
  RUN_TEST(test_ticket_fifo);
  RUN_TEST(test_fmutex_mutual_exclusion);
  RUN_TEST(test_fmutex_waiters_sleep);
  return test_summary();
}
