/*
 * replace.c - Module 08, part C: reference solution.
 */
#include "replace.h"

/* Index of page in frame[], or -1. */
static int find(const int *frame, int nframes, int page)
{
  for (int f = 0; f < nframes; f++)
    if (frame[f] == page)
      return f;
  return -1;
}

static int find_empty(const int *frame, int nframes)
{
  return find(frame, nframes, -1);
}

int repl_fifo(const int *refs, int n, int nframes)
{
  int frame[REPL_MAX_FRAMES], faults = 0, next = 0;
  for (int f = 0; f < nframes; f++)
    frame[f] = -1;
  for (int i = 0; i < n; i++) {
    if (find(frame, nframes, refs[i]) >= 0)
      continue;
    faults++;
    /* Frames fill in order 0, 1, 2, ... and are evicted in the same
     * order: a circular pointer is the whole queue. */
    frame[next] = refs[i];
    next = (next + 1) % nframes;
  }
  return faults;
}

int repl_lru(const int *refs, int n, int nframes)
{
  int frame[REPL_MAX_FRAMES], last_use[REPL_MAX_FRAMES], faults = 0;
  for (int f = 0; f < nframes; f++)
    frame[f] = -1;
  for (int i = 0; i < n; i++) {
    int f = find(frame, nframes, refs[i]);
    if (f < 0) {
      faults++;
      f = find_empty(frame, nframes);
      if (f < 0) {
        f = 0;
        for (int g = 1; g < nframes; g++)
          if (last_use[g] < last_use[f])
            f = g;
      }
      frame[f] = refs[i];
    }
    last_use[f] = i;
  }
  return faults;
}

int repl_opt(const int *refs, int n, int nframes)
{
  int frame[REPL_MAX_FRAMES], faults = 0;
  for (int f = 0; f < nframes; f++)
    frame[f] = -1;
  for (int i = 0; i < n; i++) {
    if (find(frame, nframes, refs[i]) >= 0)
      continue;
    faults++;
    int f = find_empty(frame, nframes);
    if (f < 0) {
      int best_next = -1;
      for (int g = 0; g < nframes; g++) {
        int next = n;                          /* never again = infinitely far */
        for (int j = i + 1; j < n; j++)
          if (refs[j] == frame[g]) {
            next = j;
            break;
          }
        if (next > best_next) {
          best_next = next;
          f = g;
        }
      }
    }
    frame[f] = refs[i];
  }
  return faults;
}

int repl_clock(const int *refs, int n, int nframes)
{
  int frame[REPL_MAX_FRAMES], rbit[REPL_MAX_FRAMES], faults = 0, hand = 0;
  for (int f = 0; f < nframes; f++) {
    frame[f] = -1;
    rbit[f] = 0;
  }
  for (int i = 0; i < n; i++) {
    int f = find(frame, nframes, refs[i]);
    if (f >= 0) {
      rbit[f] = 1;
      continue;
    }
    faults++;
    while (frame[hand] != -1 && rbit[hand]) {   /* second chance */
      rbit[hand] = 0;
      hand = (hand + 1) % nframes;
    }
    frame[hand] = refs[i];
    rbit[hand] = 1;
    hand = (hand + 1) % nframes;
  }
  return faults;
}
