#include "check.h"
#include "bank.h"

#include <stdatomic.h>
#include <stdint.h>

#define NTHREADS 8

static unsigned next_rand(unsigned *s)
{
  *s ^= *s << 13;
  *s ^= *s >> 17;
  *s ^= *s << 5;
  return *s;
}

static void test_single_threaded(void)
{
  bank_t b;
  CHECK_EQ(bank_init(&b, 3, 100), 0);
  CHECK_EQ(bank_total(&b), 300);
  CHECK_EQ(bank_deposit(&b, 0, 50), 0);
  CHECK_EQ(bank_deposit(&b, 0, 0), -1);
  CHECK_EQ(bank_deposit(&b, 7, 10), -1);
  CHECK_EQ(bank_withdraw(&b, 1, 150), -1);
  CHECK_EQ(bank_withdraw(&b, 1, 100), 0);
  CHECK_EQ(bank_balance(&b, 1), 0);
  CHECK_EQ(bank_transfer(&b, 0, 2, 120), 0);
  CHECK_EQ(bank_transfer(&b, 0, 2, 31), -1);
  CHECK_EQ(bank_transfer(&b, 2, 2, 1), -1);
  CHECK_EQ(bank_balance(&b, 0), 30);
  CHECK_EQ(bank_balance(&b, 2), 220);
  CHECK_EQ(bank_total(&b), 250);
  bank_destroy(&b);
}

/* ------------------------------------------------------------------ */
static bank_t shared;

static void *deposit_loop(void *arg)
{
  (void)arg;
  for (int i = 0; i < 20000; i++)
    bank_deposit(&shared, 0, 1);
  return NULL;
}

static void *concurrent_deposits(void *unused)
{
  (void)unused;
  bank_init(&shared, 1, 0);
  pthread_t t[NTHREADS];
  for (int i = 0; i < NTHREADS; i++)
    pthread_create(&t[i], NULL, deposit_loop, NULL);
  for (int i = 0; i < NTHREADS; i++)
    pthread_join(t[i], NULL);
  long got = bank_balance(&shared, 0);
  CHECK(got == NTHREADS * 20000L, "balance %ld after %d deposits of 1 -> lost updates", got,
        NTHREADS * 20000);
  bank_destroy(&shared);
  return NULL;
}

static void test_concurrent_deposits(void)
{
  must_finish_within(30, "concurrent deposits", concurrent_deposits, NULL);
}

/* ------------------------------------------------------------------ */
static atomic_long successful_withdrawals;

static void *withdraw_loop(void *arg)
{
  (void)arg;
  for (int i = 0; i < 2000; i++)
    if (bank_withdraw(&shared, 0, 1) == 0)
      atomic_fetch_add(&successful_withdrawals, 1);
  return NULL;
}

static void *no_overdraft(void *unused)
{
  (void)unused;
  bank_init(&shared, 1, 5000);
  atomic_store(&successful_withdrawals, 0);
  pthread_t t[NTHREADS];
  for (int i = 0; i < NTHREADS; i++)
    pthread_create(&t[i], NULL, withdraw_loop, NULL);
  for (int i = 0; i < NTHREADS; i++)
    pthread_join(t[i], NULL);
  long ok = atomic_load(&successful_withdrawals);
  long bal = bank_balance(&shared, 0);
  CHECK(ok == 5000, "%ld withdrawals of 1 succeeded from an account holding 5000", ok);
  CHECK(bal == 0, "final balance %ld, expected 0 (negative = overdraft by check-then-act)", bal);
  bank_destroy(&shared);
  return NULL;
}

static void test_no_overdraft(void)
{
  must_finish_within(30, "concurrent withdrawals", no_overdraft, NULL);
}

/* ------------------------------------------------------------------ */
#define ACCOUNTS 10
#define START 1000

static atomic_int stop_audit;
static atomic_long bad_totals;
static atomic_long audits;

static void *transfer_loop(void *arg)
{
  unsigned seed = (unsigned)(uintptr_t)arg * 2654435761u + 1;
  for (int i = 0; i < 20000; i++) {
    size_t from = next_rand(&seed) % ACCOUNTS, to = next_rand(&seed) % ACCOUNTS;
    long amount = 1 + next_rand(&seed) % 50;
    bank_transfer(&shared, from, to, amount);
  }
  return NULL;
}

static void *auditor(void *arg)
{
  (void)arg;
  while (!atomic_load(&stop_audit)) {
    if (bank_total(&shared) != ACCOUNTS * START)
      atomic_fetch_add(&bad_totals, 1);
    atomic_fetch_add(&audits, 1);
  }
  return NULL;
}

static void *transfers_keep_total(void *unused)
{
  (void)unused;
  bank_init(&shared, ACCOUNTS, START);
  atomic_store(&stop_audit, 0);
  atomic_store(&bad_totals, 0);
  atomic_store(&audits, 0);

  pthread_t audit, t[4];
  pthread_create(&audit, NULL, auditor, NULL);
  for (uintptr_t i = 0; i < 4; i++)
    pthread_create(&t[i], NULL, transfer_loop, (void *)(i + 1));
  for (int i = 0; i < 4; i++)
    pthread_join(t[i], NULL);
  atomic_store(&stop_audit, 1);
  pthread_join(audit, NULL);

  CHECK(atomic_load(&bad_totals) == 0,
        "%ld of %ld bank_total() calls saw a total != %d: a transfer was observed half-done",
        atomic_load(&bad_totals), atomic_load(&audits), ACCOUNTS * START);
  CHECK_EQ(bank_total(&shared), ACCOUNTS * START);
  for (size_t i = 0; i < ACCOUNTS; i++)
    CHECK(bank_balance(&shared, i) >= 0, "account %zu is negative: %ld", i, bank_balance(&shared, i));
  bank_destroy(&shared);
  return NULL;
}

static void test_transfers_keep_total(void)
{
  must_finish_within(60, "transfers + auditor", transfers_keep_total, NULL);
}

int main(void)
{
  RUN_TEST(test_single_threaded);
  RUN_TEST(test_concurrent_deposits);
  RUN_TEST(test_no_overdraft);
  RUN_TEST(test_transfers_keep_total);
  return test_summary();
}
