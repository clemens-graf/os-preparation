#include "check.h"
#include "accounts.h"

#include <stdatomic.h>
#include <stdint.h>

#define N_ACC 10
#define START 1000

static accounts_t acc;

static void *single_threaded(void *unused)
{
  (void)unused;
  accounts_t a;
  CHECK_EQ(accounts_init(&a, 3, 100), 0);
  CHECK_EQ(accounts_transfer(&a, 0, 1, 60), 0);
  CHECK_EQ(accounts_transfer(&a, 0, 1, 60), -1);
  CHECK_EQ(accounts_transfer(&a, 2, 2, 10), -1);   /* from == to: must not self-deadlock */
  CHECK_EQ(accounts_transfer(&a, 0, 9, 1), -1);
  CHECK_EQ(accounts_total(&a), 300);
  accounts_destroy(&a);
  return NULL;
}

static void test_single_threaded(void)
{
  must_finish_within(5, "single-threaded transfers (from == to?)", single_threaded, NULL);
}

static unsigned rnd(unsigned *s)
{
  *s ^= *s << 13;
  *s ^= *s >> 17;
  *s ^= *s << 5;
  return *s;
}

static void *ping(void *arg)       /* 0 -> 1, over and over */
{
  (void)arg;
  for (int i = 0; i < 20000; i++)
    accounts_transfer(&acc, 0, 1, 1);
  return NULL;
}

static void *pong(void *arg)       /* 1 -> 0: the opposite order */
{
  (void)arg;
  for (int i = 0; i < 20000; i++)
    accounts_transfer(&acc, 1, 0, 1);
  return NULL;
}

static void *random_transfers(void *arg)
{
  unsigned s = (unsigned)(uintptr_t)arg * 2654435761u + 7;
  for (int i = 0; i < 20000; i++)
    accounts_transfer(&acc, rnd(&s) % N_ACC, rnd(&s) % N_ACC, 1 + rnd(&s) % 20);
  return NULL;
}

static atomic_int stop;
static atomic_long bad_totals;

static void *auditor(void *arg)
{
  (void)arg;
  while (!atomic_load(&stop))
    if (accounts_total(&acc) != N_ACC * START)
      atomic_fetch_add(&bad_totals, 1);
  return NULL;
}

static void *opposing(void *unused)
{
  (void)unused;
  accounts_init(&acc, N_ACC, START);
  atomic_store(&stop, 0);
  atomic_store(&bad_totals, 0);
  pthread_t t[10], a;
  pthread_create(&a, NULL, auditor, NULL);
  for (uintptr_t i = 0; i < 3; i++) {
    pthread_create(&t[i], NULL, ping, NULL);
    pthread_create(&t[i + 3], NULL, pong, NULL);
  }
  for (uintptr_t i = 6; i < 10; i++)
    pthread_create(&t[i], NULL, random_transfers, (void *)i);
  for (int i = 0; i < 10; i++)
    pthread_join(t[i], NULL);
  atomic_store(&stop, 1);
  pthread_join(a, NULL);
  CHECK_EQ(accounts_total(&acc), N_ACC * START);
  CHECK(atomic_load(&bad_totals) == 0, "accounts_total saw %ld inconsistent totals",
        atomic_load(&bad_totals));
  for (size_t i = 0; i < N_ACC; i++)
    CHECK(acc.acc[i].balance >= 0, "account %zu negative", i);
  accounts_destroy(&acc);
  return NULL;
}

static void test_opposing_transfers(void)
{
  must_finish_within(60, "opposing + random transfers with an auditor", opposing, NULL);
}

int main(void)
{
  RUN_TEST(test_single_threaded);
  RUN_TEST(test_opposing_transfers);
  return test_summary();
}
