/*
 * check_then_act.c - a race WITHOUT any ++ in sight.
 *
 * Lazy initialisation:   if (config == NULL) config = load_config();
 *
 * The check and the act are two separate steps. Several threads can all
 * see NULL before any of them has stored its result: the "expensive" load
 * runs several times, all but one result are leaked, and threads may use
 * different config objects. The same shape appears as
 *   - "if (file does not exist) create it"          (TOCTOU bugs)
 *   - "if (balance >= x) balance -= x"               (overdraft)
 *   - "if (thread->state == Sleeping) wake(thread)"  (lost wake-ups, module 05)
 *
 * The fix: make check+act ONE indivisible step - hold a lock across both,
 * or use pthread_once which exists exactly for this.
 */
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 8

struct config { int value; };

static atomic_int loads;                /* how often load_config ran */

static struct config *load_config(void)
{
  atomic_fetch_add(&loads, 1);
  usleep(1000);                         /* "reading a file": widens the window */
  struct config *c = malloc(sizeof *c);
  c->value = 42;
  return c;
}

/* ----------------------------- broken ----------------------------- */
static struct config *config_racy;

static void *use_racy(void *arg)
{
  (void)arg;
  if (config_racy == NULL)              /* check ...                 */
    config_racy = load_config();        /* ... act: NOT atomic together */
  return config_racy;
}

/* ----------------------------- fixed ------------------------------ */
static struct config *config_once;
static pthread_once_t once = PTHREAD_ONCE_INIT;

static void init_config(void) { config_once = load_config(); }

static void *use_once(void *arg)
{
  (void)arg;
  pthread_once(&once, init_config);     /* all callers return after init is done */
  return config_once;
}

static void run(const char *title, void *(*fn)(void *))
{
  pthread_t t[N];
  void *seen[N];
  atomic_store(&loads, 0);
  for (int i = 0; i < N; i++)
    pthread_create(&t[i], NULL, fn, NULL);
  int distinct = 0;
  for (int i = 0; i < N; i++) {
    pthread_join(t[i], &seen[i]);
    int is_new = 1;
    for (int j = 0; j < i; j++)
      if (seen[j] == seen[i])
        is_new = 0;
    distinct += is_new;
  }
  printf("%-10s load_config ran %d time(s), threads saw %d different config object(s)\n",
         title, atomic_load(&loads), distinct);
}

int main(void)
{
  run("racy:", use_racy);                 /* leaks all but one config - on purpose */
  run("pthread_once:", use_once);
  free(config_once);
  return 0;
}
