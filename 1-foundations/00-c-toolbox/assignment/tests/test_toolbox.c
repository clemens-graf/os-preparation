#include "check.h"
#include "toolbox.h"

#include <stdint.h>

/* ------------------------------ Part A ------------------------------ */

static void test_vec_empty(void)
{
  vec_t v;
  CHECK_EQ(vec_init(&v), 0);
  CHECK_EQ(v.len, 0);
  CHECK(vec_get(&v, 0) == NULL, "vec_get on an empty vector must return NULL");
  CHECK(vec_pop(&v) == NULL, "vec_pop on an empty vector must return NULL");
  vec_free(&v, NULL);
}

static void test_vec_push_get(void)
{
  vec_t v;
  if (vec_init(&v) != 0) {
    CHECK(0, "vec_init failed");
    return;
  }
  int values[100];
  for (int i = 0; i < 100; i++) {
    values[i] = i * 3;
    CHECK_EQ(vec_push(&v, &values[i]), 0);
  }
  CHECK_EQ(v.len, 100);
  CHECK(v.cap >= 100, "capacity %zu is smaller than the length", v.cap);
  CHECK(v.cap <= 128, "capacity %zu: grow by doubling starting at 4 (expected 128)", v.cap);
  for (int i = 0; i < 100; i++) {
    int *p = vec_get(&v, i);
    CHECK(p == &values[i], "vec_get(%d) returned the wrong pointer", i);
  }
  CHECK(vec_get(&v, 100) == NULL, "index == len is out of range");
  CHECK(vec_get(&v, (size_t)-1) == NULL, "huge index is out of range");
  vec_free(&v, NULL);
  CHECK_EQ(v.len, 0);
}

static void test_vec_pop_order(void)
{
  vec_t v;
  if (vec_init(&v) != 0) {
    CHECK(0, "vec_init failed");
    return;
  }
  int a = 1, b = 2, c = 3;
  vec_push(&v, &a);
  vec_push(&v, &b);
  vec_push(&v, &c);
  CHECK(vec_pop(&v) == &c, "pop must return the last pushed item");
  CHECK(vec_pop(&v) == &b, "pop must return items in LIFO order");
  CHECK_EQ(v.len, 1);
  CHECK(vec_get(&v, 1) == NULL, "popped slot must be out of range now");
  vec_free(&v, NULL);
}

static void sum_cb(void *item, void *ctx)
{
  *(long *)ctx += *(int *)item;
}

static void order_cb(void *item, void *ctx)
{
  int **cursor = ctx;
  **cursor = *(int *)item;   /* record the visiting order */
  (*cursor)++;
}

static void test_vec_foreach(void)
{
  vec_t v;
  if (vec_init(&v) != 0) {
    CHECK(0, "vec_init failed");
    return;
  }
  int values[10];
  for (int i = 0; i < 10; i++) {
    values[i] = i + 1;
    vec_push(&v, &values[i]);
  }
  long sum = 0;
  vec_foreach(&v, sum_cb, &sum);
  CHECK_EQ(sum, 55);

  int seen[10] = {0};
  int *cursor = seen;
  vec_foreach(&v, order_cb, &cursor);
  for (int i = 0; i < 10; i++)
    CHECK_EQ(seen[i], i + 1);
  vec_free(&v, NULL);
}

static void test_vec_owns_items(void)
{
  /* The vector owns heap items: vec_free(v, free) must release them.
   * If it does not, LeakSanitizer reports a leak at program exit. */
  vec_t v;
  if (vec_init(&v) != 0) {
    CHECK(0, "vec_init failed");
    return;
  }
  for (int i = 0; i < 20; i++) {
    int *p = malloc(sizeof *p);
    *p = i;
    if (vec_push(&v, p) != 0)
      free(p);
  }
  vec_free(&v, free);
  CHECK(v.items == NULL && v.len == 0 && v.cap == 0,
        "after vec_free the vector must be empty again (items=NULL, len=cap=0)");
}

/* ------------------------------ Part B ------------------------------ */

static void *double_it(void *arg)
{
  long *out = malloc(sizeof *out);
  *out = 2 * *(long *)arg;
  return out;
}

static void *identity(void *arg) { return arg; }

static void test_task_roundtrip(void)
{
  long in = 21;
  task_t *t = NULL;
  CHECK_EQ(task_create(&t, double_it, &in), 0);
  if (!t) {
    CHECK(0, "task_create did not store a handle in *out");
    return;
  }
  CHECK(t->fn == double_it && t->arg == &in && t->done == 0, "task not initialised correctly");
  task_run(t);
  void *res = NULL;
  CHECK_EQ(task_join(t, &res), 0);
  CHECK(res != NULL && *(long *)res == 42, "expected the result 42");
  free(res);
}

static void test_task_join_before_run(void)
{
  task_t *t = NULL;
  if (task_create(&t, identity, (void *)(intptr_t)7) != 0 || !t) {
    CHECK(0, "task_create failed");
    return;
  }
  void *res = (void *)1;
  CHECK_EQ(task_join(t, &res), -1);
  CHECK(res == (void *)1, "a failed join must not touch *result_out");
  task_run(t);
  CHECK_EQ(task_join(t, NULL), 0);   /* NULL: caller does not want the result */
}

static void test_task_bad_args(void)
{
  task_t *t = (task_t *)0x1;
  CHECK_EQ(task_create(&t, NULL, NULL), -1);
  CHECK(t == (task_t *)0x1, "on error *out must stay untouched");
  CHECK_EQ(task_create(NULL, identity, NULL), -1);
}

static void test_task_int_in_pointer(void)
{
  /* Passing a small integer *inside* the void * - common with threads. */
  task_t *tasks[5];
  for (intptr_t i = 0; i < 5; i++)
    if (task_create(&tasks[i], identity, (void *)(i * 10)) != 0) {
      CHECK(0, "task_create failed");
      return;
    }
  for (int i = 4; i >= 0; i--)
    task_run(tasks[i]);
  for (intptr_t i = 0; i < 5; i++) {
    void *r;
    CHECK_EQ(task_join(tasks[i], &r), 0);
    CHECK_EQ((intptr_t)r, i * 10);
  }
}

/* ------------------------------ Part C ------------------------------ */

static void check_split(const char *line, const char **expected, size_t n)
{
  size_t count = 999;
  char **w = split_words(line, &count);
  CHECK(w != NULL, "split_words(\"%s\") returned NULL", line);
  if (!w)
    return;
  CHECK_EQ(count, n);
  for (size_t i = 0; i < n && i < count; i++)
    CHECK_STR(w[i], expected[i]);
  if (count == n)
    CHECK(w[n] == NULL, "the array must be NULL-terminated");
  free_words(w);
}

static void test_split_basic(void)
{
  const char *e1[] = {"ls", "-l", "/tmp"};
  check_split("ls -l /tmp", e1, 3);
  const char *e2[] = {"echo", "hi"};
  check_split("   echo \t  hi  \n", e2, 2);
  const char *e3[] = {"single"};
  check_split("single", e3, 1);
}

static void test_split_empty(void)
{
  check_split("", NULL, 0);
  check_split("   \t \n", NULL, 0);
}

static void test_split_does_not_modify_input(void)
{
  char line[] = "a bb ccc";
  char **w = split_words(line, NULL);   /* count may be NULL */
  CHECK_STR(line, "a bb ccc");
  if (w) {
    CHECK((void *)w[0] != (void *)line, "words must be copies, not pointers into the input");
    free_words(w);
  } else {
    CHECK(0, "split_words returned NULL");
  }
  free_words(NULL);
}

int main(void)
{
  RUN_TEST(test_vec_empty);
  RUN_TEST(test_vec_push_get);
  RUN_TEST(test_vec_pop_order);
  RUN_TEST(test_vec_foreach);
  RUN_TEST(test_vec_owns_items);
  RUN_TEST(test_task_roundtrip);
  RUN_TEST(test_task_join_before_run);
  RUN_TEST(test_task_bad_args);
  RUN_TEST(test_task_int_in_pointer);
  RUN_TEST(test_split_basic);
  RUN_TEST(test_split_empty);
  RUN_TEST(test_split_does_not_modify_input);
  return test_summary();
}
