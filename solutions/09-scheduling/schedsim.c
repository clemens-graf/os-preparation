/*
 * schedsim.c - Module 09: reference solution.
 */
#include "schedsim.h"

#include <string.h>

/* ------------------------------ Part A ------------------------------ */

/* What the policy minimises. */
static int key(enum policy policy, const struct job *jobs, const int *remaining, int i)
{
  switch (policy) {
  case SJF:  return jobs[i].burst;
  case SRTF: return remaining[i];
  case PRIO: return jobs[i].priority;
  default:   return jobs[i].arrival;        /* FCFS */
  }
}

/* Position in ready[] of the best job: smallest key, then earliest
 * arrival, then smallest index. */
static int best(enum policy policy, const struct job *jobs, const int *remaining,
                const int *ready, int nready)
{
  int b = 0;
  for (int k = 1; k < nready; k++) {
    int i = ready[k], j = ready[b];
    int ki = key(policy, jobs, remaining, i), kj = key(policy, jobs, remaining, j);
    if (ki < kj || (ki == kj && (jobs[i].arrival < jobs[j].arrival ||
                                 (jobs[i].arrival == jobs[j].arrival && i < j))))
      b = k;
  }
  return b;
}

int simulate(enum policy policy, const struct job *jobs, int n, int quantum, char *gantt)
{
  int remaining[MAX_JOBS], ready[MAX_JOBS], nready = 0;
  int running = -1, used = 0, done = 0, t = 0;
  for (int i = 0; i < n; i++)
    remaining[i] = jobs[i].burst;

  while (done < n) {
    /* 1. arrivals */
    for (int i = 0; i < n; i++)
      if (jobs[i].arrival == t)
        ready[nready++] = i;

    /* 2. the running job is done */
    if (running >= 0 && remaining[running] == 0) {
      running = -1;
      if (++done == n)
        break;
    }

    /* 3. RR: quantum used up -> behind this tick's arrivals */
    if (policy == RR && running >= 0 && used == quantum) {
      ready[nready++] = running;
      running = -1;
    }

    /* 4. preemption, only if strictly better */
    if ((policy == SRTF || policy == PRIO) && running >= 0 && nready > 0) {
      int b = ready[best(policy, jobs, remaining, ready, nready)];
      if (key(policy, jobs, remaining, b) < key(policy, jobs, remaining, running)) {
        ready[nready++] = running;
        running = -1;
      }
    }

    /* 5. pick */
    if (running < 0 && nready > 0) {
      int b = policy == RR ? 0 : best(policy, jobs, remaining, ready, nready);
      running = ready[b];
      memmove(&ready[b], &ready[b + 1], (size_t)(nready - b - 1) * sizeof ready[0]);
      nready--;
      used = 0;
    }

    /* 6. run one unit */
    if (running < 0) {
      gantt[t] = '.';
    } else {
      gantt[t] = (char)('A' + running);
      remaining[running]--;
      used++;
    }
    t++;
  }
  gantt[t] = '\0';
  return t;
}

/* ------------------------------ Part B ------------------------------ */

struct metrics measure(const struct job *jobs, int n, const char *gantt)
{
  struct metrics m = {0, 0, 0, 0};
  int len = (int)strlen(gantt);
  for (int i = 0; i < n; i++) {
    char c = (char)('A' + i);
    int first = -1, last = -1;
    for (int t = 0; t < len; t++)
      if (gantt[t] == c) {
        if (first < 0)
          first = t;
        last = t;
      }
    int turnaround = last + 1 - jobs[i].arrival;
    m.turnaround += turnaround;
    m.waiting += turnaround - jobs[i].burst;
    m.response += first - jobs[i].arrival;
  }
  if (n > 0) {
    m.turnaround /= n;
    m.waiting /= n;
    m.response /= n;
  }
  for (int t = 1; t < len; t++)
    if (gantt[t] != gantt[t - 1] && gantt[t] != '.' && gantt[t - 1] != '.')
      m.switches++;
  return m;
}

/* -------------------------- Part C (bonus): MLFQ ------------------------- */

struct fifo {
  int item[MAX_JOBS];
  int len;
};

static void push(struct fifo *q, int i)
{
  q->item[q->len++] = i;
}

static int pop(struct fifo *q)
{
  int i = q->item[0];
  memmove(&q->item[0], &q->item[1], (size_t)(q->len - 1) * sizeof q->item[0]);
  q->len--;
  return i;
}

int simulate_mlfq(const struct io_job *jobs, int n, const int quantum[MLFQ_LEVELS], int boost,
                  char *gantt)
{
  int level[MAX_JOBS], used[MAX_JOBS], cur[MAX_JOBS], left[MAX_JOBS];
  int ready_at[MAX_JOBS], finished[MAX_JOBS];       /* ready_at -1: not pending */
  struct fifo q[MLFQ_LEVELS] = {0};
  for (int i = 0; i < n; i++) {
    level[i] = used[i] = cur[i] = finished[i] = 0;
    left[i] = jobs[i].burst[0];
    ready_at[i] = jobs[i].arrival;
  }
  int running = -1, done = 0, t = 0;

  while (done < n) {
    /* 1. arrivals and I/O completions */
    for (int i = 0; i < n; i++)
      if (ready_at[i] == t) {
        push(&q[level[i]], i);
        ready_at[i] = -1;
      }

    /* 2. priority boost */
    if (boost > 0 && t > 0 && t % boost == 0) {
      for (int l = 1; l < MLFQ_LEVELS; l++)
        while (q[l].len)
          push(&q[0], pop(&q[l]));
      for (int i = 0; i < n; i++)
        if (!finished[i]) {
          level[i] = 0;
          used[i] = 0;
        }
    }

    /* 3. the running job */
    if (running >= 0) {
      int i = running;
      if (left[i] == 0) {
        running = -1;
        if (cur[i] == jobs[i].nbursts - 1) {
          finished[i] = 1;
          if (++done == n)
            break;
        } else {
          if (used[i] >= quantum[level[i]]) {       /* allotment used up */
            if (level[i] < MLFQ_LEVELS - 1)
              level[i]++;
            used[i] = 0;
          }
          ready_at[i] = t + jobs[i].burst[cur[i] + 1];
          cur[i] += 2;
          left[i] = jobs[i].burst[cur[i]];
        }
      } else if (used[i] == quantum[level[i]]) {
        running = -1;
        if (level[i] < MLFQ_LEVELS - 1)
          level[i]++;
        used[i] = 0;
        push(&q[level[i]], i);
      }
    }

    /* 4. preemption by a higher level */
    if (running >= 0) {
      for (int l = 0; l < level[running]; l++)
        if (q[l].len) {
          push(&q[level[running]], running);
          running = -1;
          break;
        }
    }

    /* 5. pick */
    if (running < 0)
      for (int l = 0; l < MLFQ_LEVELS; l++)
        if (q[l].len) {
          running = pop(&q[l]);
          break;
        }

    /* 6. run */
    if (running < 0) {
      gantt[t] = '.';
    } else {
      gantt[t] = (char)('A' + running);
      left[running]--;
      used[running]++;
    }
    t++;
  }
  gantt[t] = '\0';
  return t;
}
