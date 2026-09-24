# Module 09 — Answers

**Q1.**
| Policy | Gantt chart | avg. waiting | avg. response |
|---|---|---|---|
| FCFS | `AAAAABBBCDD` | 3.75 | 3.75 |
| SJF  | `AAAAACDDBBB` | 3.00 | 3.00 |
| SRTF | `ABCBBDDAAAA` | 2.00 | 0.25 |
| RR 2 | `AABBCAADDBA` | 4.25 | 1.50 |
| PRIO | `ABBBDDAAAAC` | 3.25 | 2.00 |

SRTF, step by step: at 1, B (3) beats A (4 left); at 2, C (1) beats B
(2 left). At 3, B (2) beats A (4). At 4, D (2) does *not* beat B (1
left). RR, at t = 2: C arrives *and* A's quantum ends, so the queue is
B, C, A (the newcomer goes first). SRTF has the lowest average waiting
time (it is optimal), RR a good response time, and the worst waiting
time.

**Q2.** Exchange argument: if a longer job runs directly before a shorter
one, swapping them shortens the shorter job's wait by the long job's
length and lengthens the long job's wait by the short job's length.
The total waiting time drops. Repeat until sorted: shortest first is
optimal. Predictions: τ₀ = 10; after 6: τ₁ = 8; after 4: τ₂ = 6;
after 6: τ₃ = 6; after 4: τ₄ = 5. The average follows the recent past,
and α sets how quickly.

**Q3.** Loss = 0.1 / (q + 0.1): about **9.1 %** at 1 ms, **1 %** at
10 ms, **0.1 %** at 100 ms. A tiny quantum wastes the CPU on switching
(and on cold caches and TLBs). A huge quantum degenerates into FCFS:
interactive jobs wait behind a long job for up to (n − 1)·q, so an
editor with 10 competitors waits a second for each keystroke.
Typical values are around 1–10 ms. A rule of thumb: most CPU bursts
should be shorter than q.

**Q4.** SJF and SRTF (long jobs, if short ones keep arriving) and
priority (low priority, if higher ones keep arriving; the test's
`ABBCCDDAAA` shows A waiting while newcomers pass). FCFS and RR cannot
starve a job, since every job eventually reaches the front. Remedies:
**aging** (priority rises with waiting time), a periodic **priority
boost** (MLFQ), or proportional-share scheduling (Linux: low weight, but
never zero).

**Q5.** An I/O-bound job gives up the CPU before its quantum ends, so it
never gets demoted. It runs briefly and often, and gets a good response
time. The CPU hogs sink. If `used` were reset on every I/O, a job could
compute for q − 1 units, do a tiny I/O, and repeat. It would stay on
level 0 forever and take almost the whole CPU (the test's gaming job
would keep level 0 and delay A far more). Counting the allotment across
I/O demotes it after q units of CPU in total, however the time is
split. The boost solves **starvation** (jobs at the bottom get CPU at
least after every boost) and adapts to **changing behaviour** (a job
that was CPU-bound and becomes interactive gets back up).

**Q6.** **Round robin**: take the first schedulable thread in the list,
then rotate the list so that it moves to the end. The timer interrupt
(`irqHandler_0`) calls `schedule()` on every tick, so **the quantum is
one timer tick**. When nothing else is schedulable, the **idle thread**
runs (it is always `Running`). It halts the CPU until the next
interrupt (`ArchCommon::idle`). `schedule()` runs in interrupt context
with interrupts off. `new`/`delete` take the kernel memory manager's
lock, which the interrupted thread might hold: a deadlock that can never
be resolved (module 06, Q5). Interrupts must be off because a timer
interrupt in the middle would call `schedule()` again while `threads_`
and `currentThread` are half updated.

**Q7.** Voluntary: `Scheduler::yield()` → `ArchThreads::yield()` →
`int $65` → `irqHandler_65` → `schedule()`. Involuntary: the timer
interrupt `irqHandler_0` → `schedule()`, which preempts whatever ran. A
joiner that loops `while (!done) yield();` causes one voluntary switch
per round, but stays *runnable*, so it keeps being scheduled for
nothing. It should cause **one** voluntary switch: go to `Sleeping` and
stay out of the run queue until the exiting thread wakes it.

**Q8.** The lost wake-up (module 05): thread T decides to sleep but has
not called `sleep()` yet. The waker sets T to `Running` (no effect,
since T is already `Running`). Then T goes to sleep and is never woken.
By waiting until T really is `Sleeping`, the wake-up cannot get lost.
The cost is busy-yielding: the waker burns scheduling rounds, and if T
never sleeps, the waker loops forever. For `pthread_join`: the joiner
takes the thread's (or process's) lock, checks "is the target already
finished?", and sleeps only if not. The exiting thread takes the same
lock, sets "finished", stores the return value and wakes the joiner.
The check and the decision to sleep must happen under that lock, and
releasing the lock together with going to sleep must be atomic, exactly
what a condition variable provides. SWEB's `Condition` (in
`common/source/kernel/Condition.cpp`) is the building block.

**Q9.** (a) A burns its **entire quantum** in every round-robin cycle,
and on one CPU the flag cannot even change while A spins, because B is
not running. Pure waste. (b) A gives the CPU away at once. It still
costs a context switch and a trip through the scheduler per round,
forever, and with many yielders the machine is busy doing nothing.
(c) costs nothing until B wakes A: exactly two switches. In a kernel,
only (c) is acceptable for anything but very short waits. A brief spin
(like SWEB's `SpinLock`, which yields) is tolerable for locks held for
a few instructions. In user space the same holds, and (a) is only
defensible on a multi-core machine for waits shorter than a context
switch.
