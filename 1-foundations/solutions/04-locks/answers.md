# Module 04 — Answers

**Q1.** Mutual exclusion (at most one thread inside), progress (if the
critical section is free and someone wants in, someone gets in — no
deadlock by the protocol itself), bounded waiting (a waiting thread gets
in after a bounded number of other entries — no starvation). A TAS
spinlock does not guarantee bounded waiting: an unlucky thread can lose
the race for the lock forever. The ticket lock guarantees it (FIFO).

**Q2.** On one CPU, the only way another thread can run in the middle of
your update is a context switch, and context switches are triggered by
interrupts (timer). No interrupts → no switch → nobody else runs. On a
multicore machine, other CPUs keep running regardless of *your* CPU's
interrupt flag, so they can enter the same critical section. User
programs cannot disable interrupts because it is a privileged operation
(it would let any program freeze the machine).

**Q3.** Every test-and-set is a *write*: the cache-coherence protocol must
move the cache line exclusively to that core, invalidating everyone else's
copy. N spinning cores make the line bounce between caches continuously —
which also slows down the owner's unlock. Test-and-test-and-set spins on a
*read*: all waiters share a read-only copy of the line and generate no
traffic until the owner writes 0.

**Q4.** Spinning pays off when the wait is shorter than the cost of
sleeping and waking (two context switches, a syscall) — i.e. short
critical sections, the owner currently running on another core. It is
strictly worse when the owner is *not* running: every cycle spent spinning
is a cycle the owner cannot use. On a single CPU the owner is never running
while you spin, so pure spinning is always wasted until the next timer
interrupt.

**Q5.** The waiting thread would busy-loop until its time slice ends
(the next timer interrupt). Only then could the owner run and release the
lock. Every contended acquire would waste a full time slice. Yielding hands
the CPU over immediately, so the owner can finish sooner.

**Q6.** The lock is handed to a *specific* next thread. If that thread is
preempted (not running), the lock stays unusable — nobody else may take it,
even though other threads are running and waiting. Every handoff can cost
a scheduling round (convoy effect). A plain spinlock lets whichever thread
is currently running grab it.

**Q7.** Acquire (on `lock`): no memory access of the critical section can
move *before* taking the lock. Release (on `unlock`): no memory access of
the critical section can move *after* releasing it. Together, everything
written inside the critical section is visible to the next owner. With a
plain store the compiler or CPU could, for example, move a write of
`counter` after the store that frees the lock — the next owner would read
a stale value.

**Q8.** Each thread's store `flag[i] = 1` sat in its core's store buffer
while the following load of `flag[1-i]` already read the *old* value 0
from the cache (x86 allows a later load to pass an earlier store to a
different address). So both threads saw "the other one does not want in"
and entered; a sequentially-consistent store (or `mfence`) forbids that
reordering.
