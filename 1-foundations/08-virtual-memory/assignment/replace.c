/*
 * replace.c - Module 08 assignment, part C. Replace the TODOs.
 * Test:  make test-replace
 *
 * A plain array `int frame[REPL_MAX_FRAMES]` (-1 = empty) plus one extra
 * array per algorithm (load time, last use, reference bit) is enough.
 */
#include "replace.h"

int repl_fifo(const int *refs, int n, int nframes)
{
  /* TODO */
  return -1;
}

int repl_lru(const int *refs, int n, int nframes)
{
  /* TODO */
  return -1;
}

int repl_opt(const int *refs, int n, int nframes)
{
  /* TODO: for each resident page, find its next use after position i. */
  return -1;
}

int repl_clock(const int *refs, int n, int nframes)
{
  /* TODO: follow the rules in replace.h exactly - the tests count faults. */
  return -1;
}
