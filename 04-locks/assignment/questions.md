# Module 04 — Paper questions

Answers: `../../solutions/04-locks/answers.md`.

**Q1.** Name the three requirements for a correct critical-section
solution. Which one does a plain test-and-set spinlock *not* guarantee?
Which lock from this module does?

**Q2.** On a single-CPU kernel, "disable interrupts, do the update, enable
interrupts" works as a lock. Why? Why does it stop working on a multicore
machine, and why can user programs not use it at all?

**Q3.** Why is test-and-*test*-and-set faster than plain test-and-set when
many threads wait for the same lock?

**Q4.** When is spinning better than sleeping, and when is it strictly
worse? What does that imply for a machine with one CPU?

**Q5.** SWEB's `SpinLock::acquire` calls `Scheduler::yield()` inside its
waiting loop. What would happen on SWEB (one CPU) without that call?

**Q6.** A ticket lock is fair. Why can it be much *slower* than a plain
spinlock when there are more threads than CPUs?

**Q7.** What do *acquire* and *release* semantics mean for `lock()` and
`unlock()`? What could go wrong if `unlock` were a plain, non-atomic store
the compiler may reorder?

**Q8.** In `peterson.c` both threads were inside the critical section at the
same time although the algorithm is proven correct. Explain in two
sentences what the CPU did.
