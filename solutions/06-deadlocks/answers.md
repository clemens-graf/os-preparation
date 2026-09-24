# Module 06 — Answers

**Q1.**
| Condition | Break it by | Practical? |
|---|---|---|
| Mutual exclusion | making resources shareable (read-only data, lock-free structures) | only for some resources |
| Hold and wait | acquiring all locks at once, or releasing everything before waiting (trylock + back off) | all-at-once needs knowing the set in advance; back-off risks livelock |
| No preemption | taking resources away (roll back a transaction) | rarely possible for locks |
| Circular wait | a global lock order | **the standard technique** in kernels (documented lock hierarchies) |

**Q2.** Chain T1 → B (held by T2) → C (held by T3). T3 is not waiting, so
it can finish and release C, then T2, then T1: no deadlock. If T3 requests
A (held by T1) the chain closes into a cycle T1 → T2 → T3 → T1: with
single-instance resources, a cycle means deadlock.

**Q3.** Deadlock: threads block forever waiting for each other (two
opposite lock orders). Livelock: threads keep running but make no progress
(two threads that each trylock, fail, release, retry — in lockstep forever;
two people stepping aside in a corridor). Starvation: the system makes
progress, but one thread never gets its turn (a writer with a
reader-preferring rwlock; an unlucky spinlock waiter).

**Q4.** Not a false alarm in general: two code paths take A and B in
opposite orders, and with the right timing they deadlock. It *can* be a
false alarm if something else prevents the two paths from ever running
concurrently — e.g. both run only while holding a third "outer" lock
(a "gate lock"), or one path runs only during initialisation before any
other thread exists. Real lockdeps know about such cases (lock classes,
nesting annotations).

**Q5.** The handler spins forever: the lock holder cannot run again until
the handler returns (same CPU), and the handler never returns — a deadlock
with yourself. Kernels disable interrupts (at least the conflicting ones)
while holding locks that interrupt handlers also take
(`spin_lock_irqsave` in Linux), or forbid handlers to take such locks at
all. SWEB follows the second rule: code that may run in interrupt context
must not take ordinary locks — see the comments "DO NOT use new / delete in
this Method" in `Thread::kill` and `Scheduler::schedule` (`new`/`delete`
take the kernel memory manager's lock).

**Q6.** A high-priority task waits for a lock held by a low-priority task,
while a medium-priority task (which needs no lock) preempts the
low-priority one — so effectively the high-priority task waits for the
medium one. On Mars Pathfinder this caused missed deadlines and watchdog
resets. Priority inheritance: while a task holds a lock that a
higher-priority task waits for, it temporarily runs with that higher
priority, so medium tasks cannot preempt it.

**Q7.** With `held_by_` a lock knows its owner: detect recursive locking
(the current thread already holds it), releases by a non-owner, and
destroying a thread that still holds locks (see `~Thread`). With the
per-thread holding list plus the lock each thread waits on
(`lock_waiting_on_`), the kernel can walk the wait-for graph —
"thread T1 waits on lock L held by T2, which waits on ... held by T1" —
and detect a real deadlock at the moment it would happen, instead of
silently hanging.
