# Module 09 — Scheduling

**Time:** 3 h  ·  **Theory:** [theory.pdf](theory.pdf) (handbook chapter 9)

Every thread `pthread_create` adds ends up in SWEB's scheduler list, and
every `pthread_join` decides whether a thread burns CPU time or sleeps.
Scheduling is also a classic exam topic: given jobs, draw the Gantt
chart. This module turns the exam exercise into code with exact rules,
so you can check your hand-drawn charts.

## Steps

1. **Read** chapter 9 (CPU/I-O bursts, metrics, FCFS, SJF, SRTF, RR,
   priorities and starvation, MLFQ, proportional share and Linux's
   scheduler, SWEB's round robin, busy waiting vs. sleeping).
2. **Examples** — `cd example && make run`:
   - `nice_share.c` — one CPU, four processes, shares by nice weight
   - `wakeup_latency.c` — a sleeper stays responsive next to CPU hogs
   - `ctxt_switches.c` — voluntary vs. involuntary context switches
3. **Assignment** — `assignment/schedsim.c`:
   - Part A `simulate`: FCFS, SJF, SRTF, RR and preemptive priority as
     Gantt charts, following the rules in `schedsim.h` to the letter
   - Part B `measure`: turnaround, waiting, response time, switches
   - Part C (bonus) `simulate_mlfq`: a multi-level feedback queue with
     I/O bursts, anti-gaming accounting and priority boosts
   ```bash
   cd assignment && make test
   ```
   Besides hand-picked charts, the tests compare 200 random workloads
   per policy against a reference implementation.
4. **Paper questions** — `assignment/questions.md` (Q1 is an exam-style
   exercise; Q6–Q9 are about SWEB and `pthread_join`).

## Done when

- `make test`: 7 passed (9 with the bonus).
- You can draw a Gantt chart for any of the five policies by hand and
  explain why a joining thread must sleep instead of yielding in a loop.

Reference solution and answers: `../solutions/09-scheduling/`.
