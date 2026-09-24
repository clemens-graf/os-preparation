# Module 06 — Paper questions

Answers: `../../solutions/06-deadlocks/answers.md`.

**Q1.** Name the four Coffman conditions. For each, give one technique that
breaks it (and a reason why it is or is not practical in an OS kernel).

**Q2.** Draw the resource-allocation graph for: T1 holds A and waits for B,
T2 holds B and waits for C, T3 holds C. Is there a deadlock? What if T3 now
requests A?

**Q3.** What is the difference between deadlock, livelock and starvation?
Give an example of each.

**Q4.** Your lock-order validator reports "possible deadlock: B -> A -> B",
but the program has never hung. Is the report a false alarm? When *could*
it be one?

**Q5.** A thread holds a spinlock and is interrupted; the interrupt handler
tries to take the same spinlock. What happens on a single CPU? How do
kernels prevent it?

**Q6.** What is priority inversion? Why did it matter for the Mars
Pathfinder, and what is priority inheritance?

**Q7.** In SWEB, `Thread::holding_lock_list_` and `Lock::held_by_` exist.
Which deadlock-related checks do they make possible?
