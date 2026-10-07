/*
 * lockdep.c - Module 06 assignment, part C. Replace the TODOs.
 * Test:  make test-lockdep
 *
 * Suggested data structures (all provided below):
 *   - graph: a boolean adjacency matrix edge[from][to] over lock ids,
 *     protected by graph_lock (an ordinary, untracked mutex);
 *   - names[id]: the lock name for printing;
 *   - per thread: an array of held lock ids - `_Thread_local` makes one
 *     copy of a variable per thread, exactly what "locks held by the
 *     current thread" needs (SWEB stores it in the Thread object).
 */
#include "lockdep.h"

#include <stdio.h>
#include <string.h>

static pthread_mutex_t graph_lock = PTHREAD_MUTEX_INITIALIZER;
static unsigned char edge[LD_MAX_LOCKS][LD_MAX_LOCKS];
static const char *names[LD_MAX_LOCKS];
static int next_id;

static _Thread_local int held[LD_MAX_HELD];
static _Thread_local int nheld;

static void (*report_fn)(const char *msg);

static void report(const char *msg)
{
  if (report_fn)
    report_fn(msg);
  else
    fprintf(stderr, "lockdep: %s\n", msg);
}

void ld_set_report(void (*fn)(const char *msg))
{
  report_fn = fn;
}

void ld_reset(void)
{
  pthread_mutex_lock(&graph_lock);
  memset(edge, 0, sizeof edge);
  memset(names, 0, sizeof names);
  next_id = 0;
  pthread_mutex_unlock(&graph_lock);
  nheld = 0;
}

void ld_init(ld_mutex_t *l, const char *name)
{
  /* An error-checking mutex returns EDEADLK instead of hanging if a
   * thread locks it twice - handy while your recursion check is missing. */
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&l->m, &attr);
  pthread_mutexattr_destroy(&attr);

  pthread_mutex_lock(&graph_lock);
  l->id = next_id++;
  l->name = name;
  names[l->id] = name;
  pthread_mutex_unlock(&graph_lock);
}

void ld_lock(ld_mutex_t *l)
{
  /* TODO:
   * 1. If this thread already holds l: report "recursive locking: <name>"
   *    and return without locking.
   * 2. Under graph_lock, for every held lock h without an edge h -> l yet:
   *    search a path l -> ... -> h (DFS/BFS over edge[][]); if one exists,
   *    report the cycle in the documented format. Then add edge h -> l.
   * 3. Lock l->m and push l->id onto held[].
   * Hint for the report: record each node's predecessor during the search,
   * then walk back from h to rebuild the path. */
  pthread_mutex_lock(&l->m);
}

void ld_unlock(ld_mutex_t *l)
{
  /* TODO: remove l->id from held[] (it need not be the last element -
   * unlock order may differ from lock order), then unlock. */
  pthread_mutex_unlock(&l->m);
}
