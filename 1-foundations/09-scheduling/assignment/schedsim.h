/*
 * schedsim.h - Module 09 assignment: CPU scheduling on paper, in code.
 *
 * Exam questions and the tests here use the same model: time runs in
 * whole units, the scheduler decides at every integer time t which job
 * runs during [t, t+1). The result is a Gantt chart - one character per
 * time unit, 'A' for jobs[0], 'B' for jobs[1], ..., '.' when the CPU idles.
 *
 * ---------------------------- the exact rules ----------------------------
 * At every time t, in this order:
 *   1. Jobs arriving at t become ready (appended in index order).
 *   2. If the running job has no work left, it finishes (finish time t).
 *   3. RR: if the running job has used its whole quantum, it goes to the
 *      END of the ready queue - behind the jobs that arrived at t.
 *   4. SRTF / PRIO: if a ready job is strictly better than the running
 *      job, the running job goes back to the ready jobs (preemption).
 *      A tie never preempts.
 *   5. If the CPU is free, pick a ready job:
 *        FCFS  earliest arrival
 *        SJF   shortest burst (non-preemptive)
 *        SRTF  shortest REMAINING time
 *        RR    the front of the ready queue (FIFO)
 *        PRIO  smallest priority number (preemptive)
 *      Ties (except RR): earlier arrival first, then smaller index.
 *   6. The chosen job runs for one time unit.
 * The chart ends when the last job finishes.
 */
#pragma once

#define MAX_JOBS 26

struct job {
  int arrival;       /* >= 0 */
  int burst;         /* CPU time needed, >= 1 */
  int priority;      /* smaller = more important; only used by PRIO */
};

enum policy { FCFS, SJF, SRTF, RR, PRIO };

/* ------------------------------ Part A ------------------------------ */

/*
 * Simulate; write the Gantt chart into gantt (NUL-terminated; the buffer
 * holds at least max(arrival) + sum(burst) + 1 characters). quantum is
 * only used by RR (>= 1). Returns the chart's length.
 */
int simulate(enum policy policy, const struct job *jobs, int n, int quantum, char *gantt);

/* ------------------------------ Part B ------------------------------ */

struct metrics {
  double turnaround;   /* average of finish - arrival */
  double waiting;      /* average of turnaround - burst: time spent ready, not running */
  double response;     /* average of first run - arrival */
  int switches;        /* t with gantt[t-1] != gantt[t], neither of them '.' */
};

/* Compute the metrics of a schedule from its Gantt chart. */
struct metrics measure(const struct job *jobs, int n, const char *gantt);

/* -------------------------- Part C (bonus): MLFQ ------------------------- */

#define MLFQ_LEVELS 3
#define MAX_BURSTS 31

/* A job that alternates CPU and I/O: burst[0] CPU, burst[1] I/O,
 * burst[2] CPU, ..., burst[nbursts-1] CPU (nbursts is odd). During its
 * I/O the job does not need the CPU; every job has its own device. */
struct io_job {
  int arrival;
  int nbursts;
  int burst[MAX_BURSTS];
};

/*
 * Multi-level feedback queue with levels 0 (highest) .. MLFQ_LEVELS-1,
 * round robin within each level. Each job has a level and `used`, the CPU
 * time it has used at that level. At every time t, in this order:
 *   1. Jobs arriving at t (they start at level 0) and jobs whose I/O ends
 *      at t become ready: appended, in index order, to their level's queue.
 *   2. Priority boost: if boost > 0 and t is a positive multiple of boost,
 *      every unfinished job gets level 0 and used = 0; the new level-0
 *      queue is old level 0, then old level 1, then old level 2.
 *   3. The running job:
 *      - burst done and it was the last: finished at t;
 *      - burst done, I/O follows: if used >= quantum[level], demote it
 *        (level + 1, at most the lowest; used = 0). It is blocked for
 *        the I/O burst and becomes ready at t + I/O length;
 *      - used == quantum[level]: demote it the same way and append it to
 *        its new level's queue.
 *      (Counting `used` across I/O means a job cannot stay on top by
 *      blocking just before its quantum runs out.)
 *   4. If a queue above the running job's level is non-empty, the running
 *      job is preempted: appended to its own level's queue, keeping used.
 *   5. If the CPU is free, pick the front of the highest non-empty queue.
 *   6. The chosen job runs for one time unit (used + 1).
 * Returns the chart's length, or -1 if not implemented.
 */
int simulate_mlfq(const struct io_job *jobs, int n, const int quantum[MLFQ_LEVELS], int boost,
                  char *gantt);
