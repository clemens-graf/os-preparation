/*
 * schedsim.c - Module 09 assignment. Replace the TODOs.
 * Test:  make test
 *
 * Suggested state for simulate(): remaining[] per job, the running job
 * (-1 = none) and how long it has run in its current quantum, and the
 * ready jobs as an array used as a FIFO queue (RR needs the order; the
 * other policies search it for the best job).
 */
#include "schedsim.h"

#include <string.h>

/* ------------------------------ Part A ------------------------------ */

int simulate(enum policy policy, const struct job *jobs, int n, int quantum, char *gantt)
{
  /* TODO: loop t = 0, 1, 2, ... until every job has finished, applying
   * the steps from schedsim.h in exactly that order. */
  gantt[0] = '\0';
  return 0;
}

/* ------------------------------ Part B ------------------------------ */

struct metrics measure(const struct job *jobs, int n, const char *gantt)
{
  /* TODO: for job i, find the first and the last position of 'A' + i. */
  struct metrics m = {0, 0, 0, 0};
  return m;
}

/* -------------------------- Part C (bonus): MLFQ ------------------------- */

int simulate_mlfq(const struct io_job *jobs, int n, const int quantum[MLFQ_LEVELS], int boost,
                  char *gantt)
{
  /* TODO (bonus): one FIFO queue per level; per job: level, used, index
   * of the current burst, what is left of it, and when its I/O ends. */
  return -1;
}
