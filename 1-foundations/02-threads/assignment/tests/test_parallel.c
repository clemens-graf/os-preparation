#include "check.h"
#include "parallel.h"

#include <stdatomic.h>
#include <stdint.h>

/* ---- count thread creation/joins made by parallel.c (via ld --wrap) ---- */
static atomic_int creates, joins;

int __real_pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
int __real_pthread_join(pthread_t, void **);

int __wrap_pthread_create(pthread_t *t, const pthread_attr_t *attr, void *(*fn)(void *), void *arg)
{
  atomic_fetch_add(&creates, 1);
  return __real_pthread_create(t, attr, fn, arg);
}

int __wrap_pthread_join(pthread_t t, void **ret)
{
  atomic_fetch_add(&joins, 1);
  return __real_pthread_join(t, ret);
}

static void reset_counts(void)
{
  atomic_store(&creates, 0);
  atomic_store(&joins, 0);
}

static void check_counts(size_t nthreads, const char *fn)
{
  CHECK(atomic_load(&creates) == (int)nthreads,
        "%s created %d thread(s), expected exactly %zu", fn, atomic_load(&creates), nthreads);
  CHECK(atomic_load(&joins) == atomic_load(&creates),
        "%s created %d thread(s) but joined %d", fn, atomic_load(&creates), atomic_load(&joins));
}

/* ---------------------------------------------------------------------- */

static int *random_array(size_t n, unsigned seed, int lo, int hi)
{
  int *a = malloc((n ? n : 1) * sizeof *a);
  for (size_t i = 0; i < n; i++) {
    seed = seed * 1103515245u + 12345u;
    a[i] = lo + (int)((seed >> 8) % (unsigned)(hi - lo + 1));
  }
  return a;
}

static void test_chunk_bounds_example(void)
{
  size_t b, e;
  chunk_bounds(10, 3, 0, &b, &e);
  CHECK(b == 0 && e == 4, "chunk 0 of (10, 3): got [%zu,%zu), expected [0,4)", b, e);
  chunk_bounds(10, 3, 1, &b, &e);
  CHECK(b == 4 && e == 7, "chunk 1 of (10, 3): got [%zu,%zu), expected [4,7)", b, e);
  chunk_bounds(10, 3, 2, &b, &e);
  CHECK(b == 7 && e == 10, "chunk 2 of (10, 3): got [%zu,%zu), expected [7,10)", b, e);
}

static void test_chunk_bounds_properties(void)
{
  for (size_t n = 0; n <= 40; n++) {
    for (size_t t = 1; t <= 12; t++) {
      size_t expect_begin = 0, min = (size_t)-1, max = 0;
      for (size_t i = 0; i < t; i++) {
        size_t b, e;
        chunk_bounds(n, t, i, &b, &e);
        if (b != expect_begin || e < b || e > n) {
          CHECK(0, "n=%zu nthreads=%zu chunk %zu = [%zu,%zu) is not contiguous/in range", n, t, i, b, e);
          return;
        }
        expect_begin = e;
        if (e - b < min) min = e - b;
        if (e - b > max) max = e - b;
      }
      if (expect_begin != n) {
        CHECK(0, "n=%zu nthreads=%zu: chunks cover [0,%zu) instead of [0,%zu)", n, t, expect_begin, n);
        return;
      }
      if (max - min > 1) {
        CHECK(0, "n=%zu nthreads=%zu: chunk sizes differ by %zu (max 1 allowed)", n, t, max - min);
        return;
      }
    }
  }
}

static void test_sum(void)
{
  size_t sizes[] = {0, 1, 2, 7, 1000, 100003};
  size_t threads[] = {1, 2, 3, 8, 17};
  for (size_t si = 0; si < sizeof sizes / sizeof *sizes; si++) {
    int *a = random_array(sizes[si], (unsigned)si + 1, -1000, 1000);
    long long expected = 0;
    for (size_t i = 0; i < sizes[si]; i++)
      expected += a[i];
    for (size_t ti = 0; ti < sizeof threads / sizeof *threads; ti++) {
      reset_counts();
      long long got = par_sum(a, sizes[si], threads[ti]);
      CHECK(got == expected, "par_sum(n=%zu, nthreads=%zu) = %lld, expected %lld",
            sizes[si], threads[ti], got, expected);
      check_counts(threads[ti], "par_sum");
    }
    free(a);
  }
}

static void test_max_index(void)
{
  int *a = random_array(50000, 7, 0, 1000000);
  size_t expected = 0;
  for (size_t i = 1; i < 50000; i++)
    if (a[i] > a[expected])
      expected = i;
  for (size_t t = 1; t <= 9; t++) {
    size_t got = 999999;
    reset_counts();
    CHECK_EQ(par_max_index(a, 50000, t, &got), 0);
    CHECK(got == expected, "nthreads=%zu: got index %zu, expected %zu", t, got, expected);
    check_counts(t, "par_max_index");
  }
  free(a);

  size_t out = 12345;
  CHECK_EQ(par_max_index(NULL, 0, 4, &out), -1);
  CHECK(out == 12345, "n == 0 must not touch *out");
}

static void test_max_index_ties(void)
{
  /* All equal: the smallest index (0) must win, whatever the chunking. */
  int same[37];
  for (int i = 0; i < 37; i++)
    same[i] = 5;
  for (size_t t = 1; t <= 10; t++) {
    size_t got = 999;
    par_max_index(same, 37, t, &got);
    CHECK(got == 0, "all-equal array, nthreads=%zu: got %zu, expected 0", t, got);
  }
  /* Max in two chunks: the earlier one must win. */
  int two[] = {1, 9, 2, 3, 4, 9, 0, 0};
  for (size_t t = 1; t <= 8; t++) {
    size_t got = 999;
    par_max_index(two, 8, t, &got);
    CHECK(got == 1, "nthreads=%zu: got %zu, expected 1", t, got);
  }
}

/* ---- predicate that records which threads call it ---- */
#define MAX_SEEN 64
struct pred_ctx {
  pthread_mutex_t lock;       /* the TEST may lock; your code may not */
  pthread_t seen[MAX_SEEN];
  int nseen;
  long calls;
  int divisor;
};

static int divisible(int x, void *p)
{
  struct pred_ctx *c = p;
  pthread_mutex_lock(&c->lock);
  c->calls++;
  int known = 0;
  for (int i = 0; i < c->nseen; i++)
    if (pthread_equal(c->seen[i], pthread_self()))
      known = 1;
  if (!known && c->nseen < MAX_SEEN)
    c->seen[c->nseen++] = pthread_self();
  pthread_mutex_unlock(&c->lock);
  return x % c->divisor == 0;
}

static void test_count_if(void)
{
  size_t n = 30000;
  int *a = random_array(n, 99, 0, 100000);
  size_t expected = 0;
  for (size_t i = 0; i < n; i++)
    expected += a[i] % 7 == 0;

  for (size_t t = 1; t <= 8; t++) {
    struct pred_ctx c = {.lock = PTHREAD_MUTEX_INITIALIZER, .divisor = 7};
    reset_counts();
    size_t got = par_count_if(a, n, t, divisible, &c);
    CHECK(got == expected, "nthreads=%zu: got %zu, expected %zu", t, got, expected);
    CHECK(c.calls == (long)n, "pred must be called once per element: %ld calls for %zu elements", c.calls, n);
    CHECK(c.nseen == (int)t, "pred was called from %d distinct thread(s), expected %zu", c.nseen, t);
    int from_main = 0;
    for (int i = 0; i < c.nseen; i++)
      from_main |= pthread_equal(c.seen[i], pthread_self());
    CHECK(!from_main, "pred was called from the calling thread - the work belongs in the workers");
    check_counts(t, "par_count_if");
  }
  free(a);
}

static void test_more_threads_than_elements(void)
{
  int a[] = {4, 8, 15, 16, 23, 42};
  reset_counts();
  CHECK_EQ(par_sum(a, 6, 20), 108);
  check_counts(20, "par_sum");
  size_t idx = 0;
  par_max_index(a, 6, 20, &idx);
  CHECK_EQ(idx, 5);
}

int main(void)
{
  RUN_TEST(test_chunk_bounds_example);
  RUN_TEST(test_chunk_bounds_properties);
  RUN_TEST(test_sum);
  RUN_TEST(test_max_index);
  RUN_TEST(test_max_index_ties);
  RUN_TEST(test_count_if);
  RUN_TEST(test_more_threads_than_elements);
  return test_summary();
}
