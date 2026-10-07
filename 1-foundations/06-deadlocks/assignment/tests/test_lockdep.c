#include "check.h"
#include "lockdep.h"

#include <stdint.h>

/* ---- capture reports ---- */
#define MAX_REPORTS 32
static pthread_mutex_t rep_lock = PTHREAD_MUTEX_INITIALIZER;
static char reports[MAX_REPORTS][256];
static int nreports;

static void capture(const char *msg)
{
  pthread_mutex_lock(&rep_lock);
  if (nreports < MAX_REPORTS)
    snprintf(reports[nreports++], sizeof reports[0], "%s", msg);
  pthread_mutex_unlock(&rep_lock);
}

static void fresh(void)
{
  ld_reset();
  ld_set_report(capture);
  nreports = 0;
}

static void expect_reports(const char **want, int n)
{
  CHECK(nreports == n, "got %d report(s), expected %d%s%s", nreports, n,
        nreports ? "; first: " : "", nreports ? reports[0] : "");
  for (int i = 0; i < n && i < nreports; i++)
    CHECK_STR(reports[i], want[i]);
}

static ld_mutex_t A, B, C, D;

static void setup(void)
{
  fresh();
  ld_init(&A, "A");
  ld_init(&B, "B");
  ld_init(&C, "C");
  ld_init(&D, "D");
}

/* ------------------------------------------------------------------ */
static void test_consistent_order_is_silent(void)
{
  setup();
  for (int i = 0; i < 3; i++) {
    ld_lock(&A); ld_lock(&B); ld_lock(&C);
    ld_unlock(&C); ld_unlock(&B); ld_unlock(&A);
  }
  ld_lock(&A); ld_unlock(&A);
  ld_lock(&C); ld_unlock(&C);      /* nothing held: no edges */
  ld_lock(&B); ld_lock(&D); ld_unlock(&D); ld_unlock(&B);
  expect_reports(NULL, 0);
}

static void test_simple_inversion(void)
{
  setup();
  ld_lock(&A); ld_lock(&B); ld_unlock(&B); ld_unlock(&A);
  ld_lock(&B); ld_lock(&A); ld_unlock(&A); ld_unlock(&B);
  const char *want[] = {"possible deadlock: B -> A -> B"};
  expect_reports(want, 1);
  /* the same inversion again must not be reported twice */
  ld_lock(&B); ld_lock(&A); ld_unlock(&A); ld_unlock(&B);
  expect_reports(want, 1);
}

static void test_three_lock_cycle(void)
{
  setup();
  ld_lock(&A); ld_lock(&B); ld_unlock(&B); ld_unlock(&A);   /* A -> B */
  ld_lock(&B); ld_lock(&C); ld_unlock(&C); ld_unlock(&B);   /* B -> C */
  expect_reports(NULL, 0);
  ld_lock(&C); ld_lock(&A); ld_unlock(&A); ld_unlock(&C);   /* C -> A closes the cycle */
  const char *want[] = {"possible deadlock: C -> A -> B -> C"};
  expect_reports(want, 1);
}

static void *recursive(void *unused)
{
  (void)unused;
  setup();
  ld_lock(&A);
  ld_lock(&A);                      /* must report, must not hang */
  ld_unlock(&A);
  const char *want[] = {"recursive locking: A"};
  expect_reports(want, 1);
  int rc = pthread_mutex_trylock(&A.m);
  CHECK(rc == 0, "after one unlock, A should be free again (trylock returned %d)", rc);
  if (rc == 0)
    pthread_mutex_unlock(&A.m);
  return NULL;
}

static void test_recursive_locking(void)
{
  must_finish_within(5, "recursive ld_lock", recursive, NULL);
}

static void test_unlock_out_of_order(void)
{
  /* held = {A, B}; unlocking A (not the most recent!) must leave {B}. */
  setup();
  ld_lock(&A); ld_lock(&B);        /* edge A -> B */
  ld_unlock(&A);
  ld_lock(&C);                     /* holding only B: edge B -> C, NOT A -> C */
  ld_unlock(&C); ld_unlock(&B);
  expect_reports(NULL, 0);
  ld_lock(&C); ld_lock(&B);        /* C -> B closes B -> C */
  ld_unlock(&B); ld_unlock(&C);
  const char *want[] = {"possible deadlock: C -> B -> C"};
  expect_reports(want, 1);
}

/* ---- threads: graph is global, held-lists are per thread ---- */
static void *a_then_b(void *arg)
{
  (void)arg;
  ld_lock(&A); ld_lock(&B); ld_unlock(&B); ld_unlock(&A);
  return NULL;
}

static void *b_then_a(void *arg)
{
  (void)arg;
  ld_lock(&B); ld_lock(&A); ld_unlock(&A); ld_unlock(&B);
  return NULL;
}

static void test_inversion_across_threads(void)
{
  setup();
  pthread_t t;
  pthread_create(&t, NULL, a_then_b, NULL);
  pthread_join(t, NULL);
  pthread_create(&t, NULL, b_then_a, NULL);
  pthread_join(t, NULL);
  const char *want[] = {"possible deadlock: B -> A -> B"};
  expect_reports(want, 1);
}

static pthread_barrier_t holding_a, other_done;

static void *hold_a(void *arg)
{
  (void)arg;
  ld_lock(&A);
  pthread_barrier_wait(&holding_a);
  pthread_barrier_wait(&other_done);
  ld_unlock(&A);
  return NULL;
}

static void *b_then_c(void *arg)
{
  (void)arg;
  pthread_barrier_wait(&holding_a);   /* A is held - but by ANOTHER thread */
  ld_lock(&B); ld_lock(&C); ld_unlock(&C); ld_unlock(&B);
  pthread_barrier_wait(&other_done);
  return NULL;
}

static void test_held_locks_are_per_thread(void)
{
  setup();
  pthread_barrier_init(&holding_a, NULL, 2);
  pthread_barrier_init(&other_done, NULL, 2);
  pthread_t t1, t2;
  pthread_create(&t1, NULL, hold_a, NULL);
  pthread_create(&t2, NULL, b_then_c, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  pthread_barrier_destroy(&holding_a);
  pthread_barrier_destroy(&other_done);
  /* Only B -> C exists. C -> A therefore closes no cycle. If A had been
   * treated as held by the second thread, A -> C would exist and this
   * would be (wrongly) reported. */
  ld_lock(&C); ld_lock(&A); ld_unlock(&A); ld_unlock(&C);
  expect_reports(NULL, 0);
}

int main(void)
{
  RUN_TEST(test_consistent_order_is_silent);
  RUN_TEST(test_simple_inversion);
  RUN_TEST(test_three_lock_cycle);
  RUN_TEST(test_recursive_locking);
  RUN_TEST(test_unlock_out_of_order);
  RUN_TEST(test_inversion_across_threads);
  RUN_TEST(test_held_locks_are_per_thread);
  return test_summary();
}
