/*
 * check.h - a deliberately tiny unit-test helper shared by all assignments.
 *
 *   static void test_something(void) {
 *       CHECK(x > 0, "x should be positive, got %d", x);
 *       CHECK_EQ(add(2, 3), 5);
 *       REQUIRE(p != NULL, "no p");     // CHECK, and leave the test if false
 *   }
 *   int main(void) {
 *       RUN_TEST(test_something);
 *       return test_summary();
 *   }
 *
 * CHECK / CHECK_EQ may be used from several threads at once.
 */
#pragma once

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int check_failed_  __attribute__((unused));
static int check_total_   __attribute__((unused));
static int tests_failed_  __attribute__((unused));
static int tests_skipped_ __attribute__((unused));
static int tests_run_     __attribute__((unused));
static int current_skipped_ __attribute__((unused));

#define CHECK(cond, ...)                                                        \
  do {                                                                          \
    __atomic_fetch_add(&check_total_, 1, __ATOMIC_RELAXED);                     \
    if (!(cond)) {                                                              \
      __atomic_fetch_add(&check_failed_, 1, __ATOMIC_RELAXED);                  \
      flockfile(stderr);                                                        \
      fprintf(stderr, "    FAIL %s:%d: CHECK(%s)\n         ", __FILE__,         \
              __LINE__, #cond);                                                 \
      fprintf(stderr, __VA_ARGS__);                                             \
      fputc('\n', stderr);                                                      \
      funlockfile(stderr);                                                      \
    }                                                                           \
  } while (0)

#define CHECK_EQ(actual, expected)                                              \
  do {                                                                          \
    long long a_ = (long long)(actual), e_ = (long long)(expected);             \
    CHECK(a_ == e_, "%s: got %lld, expected %lld", #actual, a_, e_);            \
  } while (0)

#define CHECK_STR(actual, expected)                                             \
  do {                                                                          \
    const char *a_ = (actual), *e_ = (expected);                                \
    CHECK(a_ != NULL && strcmp(a_, e_) == 0, "%s: got \"%s\", expected \"%s\"", \
          #actual, a_ ? a_ : "(null)", e_);                                     \
  } while (0)

/* Like CHECK, but leaves the current test when the condition is false (for
 * conditions the rest of the test depends on). Evaluates cond only once. */
#define REQUIRE(cond, ...)                                                      \
  do {                                                                          \
    int required_ = (cond);                                                     \
    CHECK(required_, __VA_ARGS__);                                              \
    if (!required_)                                                             \
      return;                                                                   \
  } while (0)

/* Leave the current test early and mark it as skipped (used for bonus parts). */
#define SKIP(...)                                                               \
  do {                                                                          \
    current_skipped_ = 1;                                                       \
    printf("    SKIP: ");                                                       \
    printf(__VA_ARGS__);                                                        \
    printf("\n");                                                               \
    return;                                                                     \
  } while (0)

static void __attribute__((unused)) run_test_(const char *name, void (*fn)(void))
{
  int before = __atomic_load_n(&check_failed_, __ATOMIC_RELAXED);
  current_skipped_ = 0;
  printf("[ RUN  ] %s\n", name);
  fflush(stdout);
  fn();
  tests_run_++;
  int failed = __atomic_load_n(&check_failed_, __ATOMIC_RELAXED) != before;
  if (failed)
    tests_failed_++;
  else if (current_skipped_)
    tests_skipped_++;
  printf("%s %s\n", failed ? "[ FAIL ]" : current_skipped_ ? "[ SKIP ]" : "[  OK  ]", name);
  fflush(stdout);
}

#define RUN_TEST(fn) run_test_(#fn, fn)

static int __attribute__((unused)) test_summary(void)
{
  printf("---- %d test(s): %d passed, %d failed, %d skipped (%d checks)\n",
         tests_run_, tests_run_ - tests_failed_ - tests_skipped_, tests_failed_,
         tests_skipped_, check_total_);
  return tests_failed_ ? 1 : 0;
}

/* ---------- helpers for concurrency tests ---------- */

static void __attribute__((unused)) sleep_ms(long ms)
{
  struct timespec ts = {ms / 1000, (ms % 1000) * 1000000L};
  while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {}
}

static long long __attribute__((unused)) now_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
}

struct deadline_job_ { void *(*fn)(void *); void *arg; };

static void *deadline_trampoline_(void *p)
{
  struct deadline_job_ *job = p;
  return job->fn(job->arg);
}

/*
 * Runs fn(arg) in a helper thread. If it does not finish within `seconds`,
 * the whole test program is terminated with a message naming `what`.
 * A hang in a concurrency test is almost always a deadlock or a lost wake-up.
 */
static void __attribute__((unused))
must_finish_within(int seconds, const char *what, void *(*fn)(void *), void *arg)
{
  struct deadline_job_ job = {fn, arg};
  pthread_t t;
  if (pthread_create(&t, NULL, deadline_trampoline_, &job) != 0) {
    perror("pthread_create");
    exit(2);
  }
  struct timespec until;
  clock_gettime(CLOCK_REALTIME, &until);
  until.tv_sec += seconds;
  if (pthread_timedjoin_np(t, NULL, &until) != 0) {
    fprintf(stderr, "    FAIL: \"%s\" did not finish within %d s -> deadlock or lost wake-up?\n",
            what, seconds);
    fflush(NULL);
    _exit(3);
  }
}
