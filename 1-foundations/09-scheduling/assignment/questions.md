# Module 09 — Paper questions

Answers: `../../solutions/09-scheduling/answers.md`.
Paths refer to your SWEB repo (`repos/osw26e3`).

**Q1.** (exam style — by hand first, then check with your simulator)
Jobs (arrival, burst, priority): A (0, 5, 2), B (1, 3, 1), C (2, 1, 3),
D (4, 2, 1). Draw the Gantt charts for FCFS, SJF, SRTF, RR with
quantum 2 and preemptive priority, using the rules in `schedsim.h`. Give
the average waiting and response time of each.

**Q2.** Why does SJF minimise the average waiting time (for jobs that
are all present at the start)? A real OS does not know the next burst
length. With exponential averaging τₙ₊₁ = α·tₙ + (1 − α)·τₙ, α = ½ and
τ₀ = 10, predict the bursts for the measured sequence 6, 4, 6, 4.

**Q3.** A context switch costs 0.1 ms. What fraction of CPU time is lost
to switching with a quantum of 1 ms, 10 ms, 100 ms (assuming every
quantum is used up)? What goes wrong at each end of the scale?

**Q4.** Which of FCFS, SJF, SRTF, RR, priority can *starve* a job? Give a
job sequence for one of them. Name two remedies.

**Q5.** MLFQ: why do I/O-bound jobs end up on high levels? Why does the
scheduler count `used` across I/O instead of resetting it, and what
would the "gaming" job in the tests achieve otherwise? What problem does
the priority boost solve?

**Q6.** Read `Scheduler::schedule()` in
`common/source/kernel/Scheduler.cpp`. Which policy is it? What is the
quantum? What runs when no thread is schedulable? Why must the function
not use `new`/`delete`, and why must interrupts be off while it runs?

**Q7.** Voluntary vs. involuntary context switch: where does each one
start in SWEB (look for `int $65` and `irqHandler_0`)? Which kind does a
thread cause that calls `pthread_join` on a thread that is still running
— and which kind *should* it cause?

**Q8.** `Scheduler::wake(t)` first yields in a loop until `t` is
`Sleeping`. Which race does that loop prevent? What does it cost? For
your P1 `pthread_join`: sketch how the joiner sleeps and who wakes it,
and which condition must be checked under which lock (module 05).

**Q9.** On SWEB's single CPU, thread A waits for a flag that thread B
sets. Compare (a) `while (!flag) {}`, (b) `while (!flag) yield();`,
(c) sleeping until B wakes A, in terms of the CPU time A wastes under
round robin. Which one is acceptable in a kernel, which one in user
space?
