#include "check.h"
#include "schedsim.h"

#include <math.h>
#include <stdint.h>

#define LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define CHART 4096

static const char *names[] = {"FCFS", "SJF", "SRTF", "RR", "PRIO"};

static void expect_chart(enum policy p, const struct job *jobs, int n, int q, const char *want)
{
  char g[CHART];
  memset(g, '?', sizeof g);
  int len = simulate(p, jobs, n, q, g);
  g[CHART - 1] = '\0';
  CHECK(strcmp(g, want) == 0, "%s%s%.0d:\n           got      \"%s\"\n           expected \"%s\"",
        names[p], p == RR ? ", quantum " : "", p == RR ? q : 0, g, want);
  CHECK(len == (int)strlen(want), "%s: returned %d, the chart has %zu units", names[p], len,
        strlen(want));
}

static int close_to(double a, double b)
{
  return fabs(a - b) < 1e-6;
}

/* ------------------------------- Part A ------------------------------- */

/* Three jobs at time 0, the long one first: the convoy effect. */
static const struct job CONVOY[] = {{0, 24, 1}, {0, 3, 1}, {0, 3, 1}};
/* Staggered arrivals (the textbook's SRTF example). */
static const struct job STAG[] = {{0, 8, 3}, {1, 4, 1}, {2, 9, 4}, {3, 5, 2}};

static void test_convoy(void)
{
  expect_chart(FCFS, CONVOY, 3, 1, "AAAAAAAAAAAAAAAAAAAAAAAABBBCCC");
  expect_chart(SJF, CONVOY, 3, 1, "BBBCCCAAAAAAAAAAAAAAAAAAAAAAAA");
  expect_chart(RR, CONVOY, 3, 4, "AAAABBBCCCAAAAAAAAAAAAAAAAAAAA");
}

static void test_staggered(void)
{
  expect_chart(FCFS, STAG, 4, 1, "AAAAAAAABBBBCCCCCCCCCDDDDD");
  expect_chart(SJF, STAG, 4, 1, "AAAAAAAABBBBDDDDDCCCCCCCCC");
  expect_chart(SRTF, STAG, 4, 1, "ABBBBDDDDDAAAAAAACCCCCCCCC");
  expect_chart(PRIO, STAG, 4, 1, "ABBBBDDDDDAAAAAAACCCCCCCCC");
}

static void test_round_robin_quanta(void)
{
  expect_chart(RR, STAG, 4, 1, "ABACBDACBDACBDACDACDACACCC");
  expect_chart(RR, STAG, 4, 2, "AABBCCAADDBBCCAADDCCAADCCC");
  expect_chart(RR, STAG, 4, 4, "AAAABBBBCCCCDDDDAAAACCCCDC");
  expect_chart(RR, STAG, 4, 100, "AAAAAAAABBBBCCCCCCCCCDDDDD");   /* huge quantum = FCFS */
  /* a newcomer and an expired quantum at the same time: newcomer first */
  static const struct job tie[] = {{0, 4, 1}, {2, 2, 1}};
  expect_chart(RR, tie, 2, 2, "AABBAA");
}

static void test_idle_and_ties(void)
{
  static const struct job gap[] = {{2, 3, 2}, {4, 2, 1}, {12, 2, 1}};
  expect_chart(FCFS, gap, 3, 1, "..AAABB.....CC");
  expect_chart(SRTF, gap, 3, 1, "..AAABB.....CC");   /* 1 left beats 2: no preemption */
  expect_chart(PRIO, gap, 3, 1, "..AABBA.....CC");   /* B is more important */
  expect_chart(RR, gap, 3, 2, "..AABBA.....CC");
  static const struct job same[] = {{0, 3, 2}, {0, 3, 2}, {1, 2, 2}};
  expect_chart(SRTF, same, 3, 1, "AAACCBBB");        /* ties: running job keeps the CPU */
  expect_chart(PRIO, same, 3, 1, "AAABBBCC");        /* equal priority: FCFS */
  expect_chart(SJF, same, 3, 1, "AAACCBBB");
}

static void test_priority(void)
{
  static const struct job p[] = {{0, 10, 3}, {0, 1, 1}, {0, 2, 4}, {0, 1, 5}, {0, 5, 2}};
  expect_chart(PRIO, p, 5, 1, "BEEEEEAAAAAAAAAACCD");
  static const struct job starve[] = {{0, 4, 5}, {1, 2, 1}, {3, 2, 1}, {5, 2, 1}};
  expect_chart(PRIO, starve, 4, 1, "ABBCCDDAAA");     /* A waits while others keep coming */
}

/* ------------------------------- Part B ------------------------------- */
static void test_metrics(void)
{
  struct metrics m = measure(CONVOY, 3, "AAAAAAAAAAAAAAAAAAAAAAAABBBCCC");
  CHECK(close_to(m.waiting, 17.0), "FCFS convoy: waiting %.3f, expected 17", m.waiting);
  CHECK(close_to(m.turnaround, 27.0), "FCFS convoy: turnaround %.3f, expected 27", m.turnaround);
  CHECK(close_to(m.response, 17.0), "FCFS convoy: response %.3f, expected 17", m.response);
  CHECK_EQ(m.switches, 2);

  m = measure(CONVOY, 3, "BBBCCCAAAAAAAAAAAAAAAAAAAAAAAA");
  CHECK(close_to(m.waiting, 3.0), "SJF: waiting %.3f, expected 3", m.waiting);

  m = measure(CONVOY, 3, "AAAABBBCCCAAAAAAAAAAAAAAAAAAAA");
  CHECK(close_to(m.waiting, 17.0 / 3), "RR 4: waiting %.3f, expected 5.667", m.waiting);
  CHECK(close_to(m.response, 11.0 / 3), "RR 4: response %.3f, expected 3.667", m.response);
  CHECK_EQ(m.switches, 3);

  m = measure(STAG, 4, "ABBBBDDDDDAAAAAAACCCCCCCCC");
  CHECK(close_to(m.waiting, 6.5), "SRTF: waiting %.3f, expected 6.5", m.waiting);
  CHECK(close_to(m.response, 4.25), "SRTF: response %.3f, expected 4.25", m.response);
  CHECK_EQ(m.switches, 4);

  static const struct job gap[] = {{2, 3, 2}, {4, 2, 1}, {12, 2, 1}};
  m = measure(gap, 3, "..AAABB.....CC");
  CHECK(close_to(m.turnaround, 8.0 / 3), "idle gaps: turnaround %.3f, expected 2.667",
        m.turnaround);
  CHECK_EQ(m.switches, 1);                           /* idle -> job is no switch */
}

/* ---------------- random workloads: exact charts + properties ---------------- */
struct lcg { uint32_t x; };

static int rnd(struct lcg *r, int m)
{
  r->x = (r->x * 1103515245u + 12345u) & 0x7fffffffu;
  return (int)((r->x >> 16) % (uint32_t)m);
}

static uint32_t fnv(uint32_t h, const char *s)
{
  for (; *s; s++) {
    h ^= (unsigned char)*s;
    h *= 16777619u;
  }
  return h;
}

/* Properties every correct schedule has, whatever the policy. */
static void check_schedule(enum policy p, const struct job *jobs, int n, const char *g,
                           unsigned seed)
{
  int len = (int)strlen(g), ran[MAX_JOBS] = {0};
  for (int t = 0; t < len; t++) {
    if (g[t] == '.') {
      for (int i = 0; i < n; i++)          /* work-conserving: idle only if nobody waits */
        if (jobs[i].arrival <= t && ran[i] < jobs[i].burst) {
          CHECK(0, "seed %u, %s: CPU idle at t=%d although %c is ready", seed, names[p], t,
                'A' + i);
          return;
        }
      continue;
    }
    int i = g[t] - 'A';
    if (i < 0 || i >= n) {
      CHECK(0, "seed %u, %s: bad character '%c' at t=%d", seed, names[p], g[t], t);
      return;
    }
    CHECK(t >= jobs[i].arrival, "seed %u, %s: %c runs at t=%d before arriving", seed,
          names[p], 'A' + i, t);
    ran[i]++;
  }
  for (int i = 0; i < n; i++)
    CHECK(ran[i] == jobs[i].burst, "seed %u, %s: %c ran %d units, needs %d", seed, names[p],
          'A' + i, ran[i], jobs[i].burst);
  if (p == FCFS || p == SJF)                /* non-preemptive: one piece per job */
    for (int i = 0; i < n; i++) {
      const char *a = strchr(g, 'A' + i), *b = strrchr(g, 'A' + i);
      CHECK(a && b - a + 1 == jobs[i].burst, "seed %u, %s: %c was preempted", seed,
            names[p], 'A' + i);
    }
}

static void test_random_workloads(void)
{
  static const uint32_t want[5] = {0x0f6542b3, 0xa1e7325f, 0xfc665f61, 0x1a55ced7, 0xf00dd201};
  uint32_t h[5] = {2166136261u, 2166136261u, 2166136261u, 2166136261u, 2166136261u};
  int failed_before = check_failed_;
  for (unsigned seed = 1; seed <= 200; seed++) {
    struct lcg r = {seed};
    struct job jobs[MAX_JOBS];
    int n = 1 + rnd(&r, 6);
    for (int i = 0; i < n; i++) {
      jobs[i].arrival = rnd(&r, 10);
      jobs[i].burst = 1 + rnd(&r, 8);
      jobs[i].priority = rnd(&r, 4);
    }
    int q = 1 + rnd(&r, 4);
    double waiting[5];
    for (int p = FCFS; p <= PRIO; p++) {
      char g[CHART];
      simulate((enum policy)p, jobs, n, q, g);
      check_schedule((enum policy)p, jobs, n, g, seed);
      waiting[p] = measure(jobs, n, g).waiting;
      h[p] = fnv(fnv(h[p], g), "|");
    }
    for (int p = FCFS; p <= PRIO; p++)     /* SRTF minimises the average waiting time */
      CHECK(waiting[SRTF] <= waiting[p] + 1e-9, "seed %u: SRTF waits %.2f on average, %s "
            "only %.2f?", seed, waiting[SRTF], names[p], waiting[p]);
    if (check_failed_ != failed_before) {
      printf("    (stopped at the first failing workload, seed %u)\n", seed);
      return;
    }
  }
  for (int p = FCFS; p <= PRIO; p++)
    CHECK(h[p] == want[p], "%s: the charts of the 200 random workloads differ from the "
          "reference (checksum %#x, expected %#x) - check the rules in schedsim.h", names[p],
          h[p], want[p]);
}

/* ---------------------------- Part C (bonus) ---------------------------- */
static const int Q[MLFQ_LEVELS] = {2, 4, 8};

static int mlfq_chart(const struct io_job *jobs, int n, int boost, char *g)
{
  memset(g, '?', CHART);
  int len = simulate_mlfq(jobs, n, Q, boost, g);
  g[CHART - 1] = '\0';
  return len;
}

static void expect_mlfq(const char *what, const struct io_job *jobs, int n, int boost,
                        const char *want)
{
  char g[CHART];
  mlfq_chart(jobs, n, boost, g);
  CHECK(strcmp(g, want) == 0, "MLFQ, %s:\n           got      \"%s\"\n           expected \"%s\"",
        what, g, want);
}

static void test_mlfq(void)
{
  char g[CHART];
  static const struct io_job one = {0, 1, {1}};
  if (mlfq_chart(&one, 1, 0, g) < 0)
    SKIP("simulate_mlfq not implemented (bonus)");

  static const struct io_job solo[] = {{0, 1, {20}}};
  expect_mlfq("a single job", solo, 1, 0, "AAAAAAAAAAAAAAAAAAAA");

  /* A burns CPU and sinks; B computes 1 unit, then waits 3 for I/O */
  static const struct io_job mix[] = {{0, 1, {12}}, {0, 7, {1, 3, 1, 3, 1, 3, 1}}};
  expect_mlfq("CPU hog + interactive job", mix, 2, 0, "AABAAABAAABAAABA");

  /* B tries to stay on top by blocking before its quantum runs out */
  static const struct io_job game[] = {{0, 1, {20}}, {0, 11, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}}};
  expect_mlfq("gaming the scheduler", game, 2, 0, "AABABAAABABABABAAAAAAAAAAA");

  /* A sinks to the bottom while short jobs keep arriving; boosts lift it */
  static const struct io_job hog[] = {{0, 1, {30}}, {4, 1, {6}}, {8, 1, {6}}, {12, 1, {6}},
                                      {16, 1, {6}}};
  expect_mlfq("no boost", hog, 5, 0, "AAAABBAACCBBDDCCEEBBDDDDCCEEEEAAAAAAAAAAAAAAAAAAAAAAAA");
  expect_mlfq("boost every 10", hog, 5, 10,
              "AAAABBAACCCCBBAADDEEEECCBBAADDDDEEAAAAAAAAAAAAAAAAAAAA");
}

static void test_mlfq_random(void)
{
  char g[CHART];
  static const struct io_job one = {0, 1, {1}};
  if (mlfq_chart(&one, 1, 0, g) < 0)
    SKIP("simulate_mlfq not implemented (bonus)");
  uint32_t h = 2166136261u;
  for (unsigned seed = 1; seed <= 200; seed++) {
    struct lcg r = {seed};
    struct io_job jobs[MAX_JOBS];
    int n = 1 + rnd(&r, 4);
    for (int i = 0; i < n; i++) {
      jobs[i].arrival = rnd(&r, 6);
      jobs[i].nbursts = 1 + 2 * rnd(&r, 4);
      for (int b = 0; b < jobs[i].nbursts; b++)
        jobs[i].burst[b] = b % 2 == 0 ? 1 + rnd(&r, 6) : 1 + rnd(&r, 4);
    }
    int boost = rnd(&r, 2) ? 15 : 0;
    mlfq_chart(jobs, n, boost, g);
    h = fnv(fnv(h, g), "|");
  }
  CHECK(h == 0x0833738fu, "MLFQ: the charts of the 200 random workloads differ from the "
        "reference (checksum %#x, expected 0x833738f)", h);
}

int main(void)
{
  RUN_TEST(test_convoy);
  RUN_TEST(test_staggered);
  RUN_TEST(test_round_robin_quanta);
  RUN_TEST(test_idle_and_ties);
  RUN_TEST(test_priority);
  RUN_TEST(test_metrics);
  RUN_TEST(test_random_workloads);
  RUN_TEST(test_mlfq);
  RUN_TEST(test_mlfq_random);
  return test_summary();
}
