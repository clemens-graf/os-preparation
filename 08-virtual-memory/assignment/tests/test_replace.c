#include "check.h"
#include "replace.h"

#define LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))

/* The classic textbook string (Silberschatz et al.) */
static const int S1[] = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};
/* Belady's string: FIFO gets WORSE with more frames */
static const int BEL[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
/* Clock gives 2 a second chance, FIFO does not */
static const int SC[] = {1, 2, 3, 4, 2, 5, 2};
static const int LONG[] = {0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1, 5, 6, 1, 2, 0, 3};

typedef int (*algo_t)(const int *, int, int);

static const struct { const char *name; algo_t fn; } algos[] = {
  {"fifo", repl_fifo}, {"lru", repl_lru}, {"opt", repl_opt}, {"clock", repl_clock},
};

static void expect(const char *what, const int *refs, int n, int nframes,
                   int fifo, int lru, int opt, int clock)
{
  int want[] = {fifo, lru, opt, clock};
  for (int a = 0; a < 4; a++) {
    int got = algos[a].fn(refs, n, nframes);
    CHECK(got == want[a], "%s, %d frames, %s: %d faults, expected %d", what, nframes,
          algos[a].name, got, want[a]);
  }
}

static void test_textbook_string(void)
{
  expect("S1", S1, LEN(S1), 1, 20, 20, 20, 20);
  expect("S1", S1, LEN(S1), 3, 15, 12, 9, 14);
  expect("S1", S1, LEN(S1), 4, 10, 8, 8, 9);
}

static void test_belady_anomaly(void)
{
  expect("Belady", BEL, LEN(BEL), 3, 9, 10, 7, 9);
  expect("Belady", BEL, LEN(BEL), 4, 10, 8, 6, 10);
  CHECK(repl_fifo(BEL, LEN(BEL), 4) > repl_fifo(BEL, LEN(BEL), 3),
        "FIFO should show Belady's anomaly on this string");
}

static void test_second_chance(void)
{
  expect("second chance", SC, LEN(SC), 3, 6, 5, 5, 5);
  expect("long", LONG, LEN(LONG), 3, 19, 16, 12, 19);
  expect("long", LONG, LEN(LONG), 4, 15, 12, 10, 15);
  expect("long", LONG, LEN(LONG), 5, 12, 10, 8, 12);
}

static void test_edge_cases(void)
{
  static const int one[] = {5};
  static const int same[] = {3, 3, 3, 3};
  for (int a = 0; a < 4; a++) {
    CHECK(algos[a].fn(one, 0, 3) == 0, "%s: empty string", algos[a].name);
    CHECK(algos[a].fn(one, 1, 1) == 1, "%s: one reference", algos[a].name);
    CHECK(algos[a].fn(same, 4, 2) == 1, "%s: the same page 4 times", algos[a].name);
    /* enough frames for everything: only cold misses */
    CHECK(algos[a].fn(LONG, LEN(LONG), 10) == 8, "%s: 8 distinct pages, 10 frames",
          algos[a].name);
    CHECK(algos[a].fn(LONG, LEN(LONG), REPL_MAX_FRAMES) == 8, "%s: %d frames",
          algos[a].name, REPL_MAX_FRAMES);
  }
}

/* ---- random strings: exact totals (from an independent implementation)
 *      and the properties every correct simulator must have ---- */
static void lcg_refs(unsigned seed, int *out, int n, int pages)
{
  unsigned x = seed;
  for (int i = 0; i < n; i++) {
    x = (x * 1103515245u + 12345u) & 0x7fffffffu;
    out[i] = (int)((x >> 16) % (unsigned)pages);
  }
}

static void test_random_strings(void)
{
  long total[4] = {0};
  int refs[200];
  for (unsigned seed = 1; seed <= 20; seed++) {
    lcg_refs(seed, refs, 200, 12);
    int prev_lru = 1 << 30, prev_opt = 1 << 30;
    for (int k = 1; k <= 12; k++) {
      int f[4];
      for (int a = 0; a < 4; a++) {
        f[a] = algos[a].fn(refs, 200, k);
        total[a] += f[a];
      }
      for (int a = 0; a < 4; a++)
        CHECK(f[2] <= f[a], "seed %u, %d frames: OPT (%d) worse than %s (%d)?", seed, k,
              f[2], algos[a].name, f[a]);
      /* LRU and OPT are stack algorithms: more frames never hurt */
      CHECK(f[1] <= prev_lru, "seed %u: LRU got worse with %d frames", seed, k);
      CHECK(f[2] <= prev_opt, "seed %u: OPT got worse with %d frames", seed, k);
      prev_lru = f[1];
      prev_opt = f[2];
    }
  }
  static const long want[4] = {23053, 23015, 16081, 23058};
  for (int a = 0; a < 4; a++)
    CHECK(total[a] == want[a], "%s: %ld faults over all random strings, expected %ld",
          algos[a].name, total[a], want[a]);
}

int main(void)
{
  RUN_TEST(test_textbook_string);
  RUN_TEST(test_belady_anomaly);
  RUN_TEST(test_second_chance);
  RUN_TEST(test_edge_cases);
  RUN_TEST(test_random_strings);
  return test_summary();
}
