/*
 * Reference solution for Module 06, part C.
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

/* Breadth-first search from `from` to `to` over the edges. On success,
 * fills path[0..len) with from ... to and returns len; 0 if unreachable.
 * Caller holds graph_lock. */
static int find_path(int from, int to, int *path)
{
  int prev[LD_MAX_LOCKS], queue[LD_MAX_LOCKS], head = 0, tail = 0;
  for (int i = 0; i < LD_MAX_LOCKS; i++)
    prev[i] = -2;                       /* -2 = not visited */
  prev[from] = -1;
  queue[tail++] = from;
  while (head < tail) {
    int u = queue[head++];
    if (u == to) {
      int rev[LD_MAX_LOCKS], n = 0;
      for (int v = to; v != -1; v = prev[v])
        rev[n++] = v;
      for (int i = 0; i < n; i++)
        path[i] = rev[n - 1 - i];
      return n;
    }
    for (int v = 0; v < next_id; v++)
      if (edge[u][v] && prev[v] == -2) {
        prev[v] = u;
        queue[tail++] = v;
      }
  }
  return 0;
}

void ld_lock(ld_mutex_t *l)
{
  char msg[512];

  for (int i = 0; i < nheld; i++) {
    if (held[i] == l->id) {             /* locking it again would self-deadlock */
      snprintf(msg, sizeof msg, "recursive locking: %s", l->name);
      report(msg);
      return;
    }
  }

  pthread_mutex_lock(&graph_lock);
  for (int i = 0; i < nheld; i++) {
    int h = held[i];
    if (edge[h][l->id])
      continue;                         /* known edge: already checked once */
    int path[LD_MAX_LOCKS];
    int len = find_path(l->id, h, path);
    if (len > 0) {                      /* l ->* h exists; h -> l closes a cycle */
      int pos = snprintf(msg, sizeof msg, "possible deadlock: %s", names[h]);
      for (int k = 0; k < len && pos < (int)sizeof msg; k++)
        pos += snprintf(msg + pos, sizeof msg - (size_t)pos, " -> %s", names[path[k]]);
      report(msg);
    }
    edge[h][l->id] = 1;
  }
  pthread_mutex_unlock(&graph_lock);

  pthread_mutex_lock(&l->m);            /* the real lock, after the checks */
  if (nheld < LD_MAX_HELD)
    held[nheld++] = l->id;
}

void ld_unlock(ld_mutex_t *l)
{
  for (int i = 0; i < nheld; i++) {
    if (held[i] == l->id) {             /* not necessarily the last one */
      held[i] = held[--nheld];
      break;
    }
  }
  pthread_mutex_unlock(&l->m);
}
