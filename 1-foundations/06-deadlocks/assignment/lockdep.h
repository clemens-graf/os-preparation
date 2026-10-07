/*
 * lockdep.h - Module 06 assignment, part C: a lock-order validator.
 *
 * Deadlocks depend on timing, so tests rarely catch them. A lock-order
 * validator finds POTENTIAL deadlocks in a single, perfectly happy run:
 *
 *   - Every thread remembers which locks it currently holds.
 *   - When a thread holding H acquires L, record the edge H -> L in a
 *     global graph ("H is taken before L").
 *   - If L -> ... -> H already exists in the graph, the new edge closes a
 *     cycle: some two code paths take these locks in opposite orders, and
 *     with unlucky timing they deadlock. Report it.
 *
 * Linux calls this "lockdep"; ThreadSanitizer has one built in; SWEB's
 * Lock class keeps per-thread holding lists for similar checks
 * (Lock::doChecksBeforeWaiting, the "lock tree" debugging tutorial).
 *
 * Report format (exact, the tests compare strings):
 *   cycle:       "possible deadlock: A -> B -> A"
 *                starts and ends with the lock the thread already HOLDS,
 *                then the lock being acquired, then the existing path back.
 *                Example: holding C, acquiring A, graph has A->B and B->C:
 *                "possible deadlock: C -> A -> B -> C"
 *   recursion:   "recursive locking: A"   (the thread already holds A;
 *                do NOT lock it again - just report and return)
 * Report each new cycle-closing edge once (when the edge is first added).
 */
#pragma once
#include <pthread.h>

#define LD_MAX_LOCKS 64
#define LD_MAX_HELD 16

typedef struct {
  pthread_mutex_t m;
  int id;              /* 0 .. LD_MAX_LOCKS-1, assigned by ld_init */
  const char *name;
} ld_mutex_t;

/* Called with the report text. Default (NULL): print to stderr. */
void ld_set_report(void (*report)(const char *msg));

/* Forget all recorded edges and lock ids (tests call this between cases;
 * no lock may be held at that moment). */
void ld_reset(void);

void ld_init(ld_mutex_t *l, const char *name);
void ld_lock(ld_mutex_t *l);
void ld_unlock(ld_mutex_t *l);
