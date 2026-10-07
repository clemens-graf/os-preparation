#include "check.h"
#include "stats.h"

#include <stdatomic.h>
#include <stdint.h>

static stats_t shared;
static atomic_int stop;
static atomic_long inconsistent, snapshots;

static void test_single_threaded(void)
{
  stats_t s;
  stats_init(&s);
  int values[] = {5, -3, 12, 7};
  for (int i = 0; i < 4; i++)
    stats_add(&s, values[i]);
  long c;
  long long sum;
  int mn, mx;
  stats_snapshot(&s, &c, &sum, &mn, &mx);
  CHECK_EQ(c, 4);
  CHECK_EQ(sum, 21);
  CHECK_EQ(mn, -3);
  CHECK_EQ(mx, 12);
  stats_destroy(&s);
}

/* Every thread adds the value 5, many times. So in every consistent
 * snapshot: sum == 5 * count, and min == max == 5 once count > 0. */
static void *add_fives(void *arg)
{
  (void)arg;
  for (int i = 0; i < 50000; i++)
    stats_add(&shared, 5);
  return NULL;
}

static void *observer(void *arg)
{
  (void)arg;
  while (!atomic_load(&stop)) {
    long c;
    long long sum;
    int mn, mx;
    stats_snapshot(&shared, &c, &sum, &mn, &mx);
    if (sum != 5LL * c || (c > 0 && (mn != 5 || mx != 5)))
      atomic_fetch_add(&inconsistent, 1);
    atomic_fetch_add(&snapshots, 1);
  }
  return NULL;
}

static void *consistent_snapshots(void *unused)
{
  (void)unused;
  stats_init(&shared);
  atomic_store(&stop, 0);
  atomic_store(&inconsistent, 0);
  atomic_store(&snapshots, 0);
  pthread_t obs, t[6];
  pthread_create(&obs, NULL, observer, NULL);
  for (int i = 0; i < 6; i++)
    pthread_create(&t[i], NULL, add_fives, NULL);
  for (int i = 0; i < 6; i++)
    pthread_join(t[i], NULL);
  atomic_store(&stop, 1);
  pthread_join(obs, NULL);

  long c;
  long long sum;
  int mn, mx;
  stats_snapshot(&shared, &c, &sum, &mn, &mx);
  CHECK(c == 6 * 50000L, "count %ld, expected %ld (lost updates)", c, 6 * 50000L);
  CHECK(sum == 5LL * 6 * 50000, "sum %lld, expected %lld (lost updates)", sum, 5LL * 6 * 50000);
  CHECK(atomic_load(&inconsistent) == 0, "%ld of %ld snapshots were inconsistent (sum != 5*count)",
        atomic_load(&inconsistent), atomic_load(&snapshots));
  stats_destroy(&shared);
  return NULL;
}

static void test_consistent_snapshots(void)
{
  must_finish_within(60, "stats with concurrent snapshots", consistent_snapshots, NULL);
}

/* Threads add disjoint ranges; min/max must end up as the global extremes. */
static void *add_range(void *arg)
{
  int base = (int)(intptr_t)arg;
  for (int i = 0; i < 20000; i++)
    stats_add(&shared, base + (i * 7919) % 20000);
  return NULL;
}

static void *min_max(void *unused)
{
  (void)unused;
  stats_init(&shared);
  pthread_t t[4];
  for (intptr_t i = 0; i < 4; i++)
    pthread_create(&t[i], NULL, add_range, (void *)(i * 20000 - 40000));
  for (int i = 0; i < 4; i++)
    pthread_join(t[i], NULL);
  long c;
  long long sum;
  int mn, mx;
  stats_snapshot(&shared, &c, &sum, &mn, &mx);
  CHECK_EQ(c, 80000);
  CHECK_EQ(mn, -40000);
  CHECK_EQ(mx, 39999);
  stats_destroy(&shared);
  return NULL;
}

static void test_min_max(void)
{
  must_finish_within(60, "concurrent min/max", min_max, NULL);
}

int main(void)
{
  RUN_TEST(test_single_threaded);
  RUN_TEST(test_consistent_snapshots);
  RUN_TEST(test_min_max);
  return test_summary();
}
