#include "check.h"
#include "dining.h"

#include <stdatomic.h>
#include <stdint.h>

#define MAX_P 16

static table_t table;
static int n_phil, meals_each;
static atomic_int eating[MAX_P];
static atomic_long neighbour_violations;
static atomic_long meals_total;

static void *philosopher(void *arg)
{
  int id = (int)(intptr_t)arg;
  int left = (id + n_phil - 1) % n_phil, right = (id + 1) % n_phil;
  for (int i = 0; i < meals_each; i++) {
    pickup(&table, id);
    atomic_store(&eating[id], 1);
    if ((left != id && atomic_load(&eating[left])) || (right != id && atomic_load(&eating[right])))
      atomic_fetch_add(&neighbour_violations, 1);
    atomic_fetch_add(&meals_total, 1);
    atomic_store(&eating[id], 0);
    putdown(&table, id);
  }
  return NULL;
}

static void *dinner(void *arg)
{
  n_phil = (int)(intptr_t)arg;
  atomic_store(&neighbour_violations, 0);
  atomic_store(&meals_total, 0);
  CHECK_EQ(table_init(&table, n_phil), 0);
  pthread_t t[MAX_P];
  for (intptr_t i = 0; i < n_phil; i++)
    pthread_create(&t[i], NULL, philosopher, (void *)i);
  for (int i = 0; i < n_phil; i++)
    pthread_join(t[i], NULL);
  table_destroy(&table);
  CHECK(atomic_load(&neighbour_violations) == 0, "%d philosophers: neighbours ate together %ld times",
        n_phil, atomic_load(&neighbour_violations));
  CHECK_EQ(atomic_load(&meals_total), (long)n_phil * meals_each);
  return NULL;
}

static void test_five(void)
{
  meals_each = 20000;
  must_finish_within(30, "5 philosophers, 20000 meals each", dinner, (void *)5);
}

static void test_two(void)
{
  meals_each = 20000;
  must_finish_within(30, "2 philosophers (both share both forks)", dinner, (void *)2);
}

static void test_eleven(void)
{
  meals_each = 10000;
  must_finish_within(30, "11 philosophers", dinner, (void *)11);
}

int main(void)
{
  RUN_TEST(test_five);
  RUN_TEST(test_two);
  RUN_TEST(test_eleven);
  return test_summary();
}
