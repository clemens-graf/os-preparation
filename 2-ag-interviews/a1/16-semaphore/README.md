# A1-16 · semaphores in user space

**Time box 25 min · ★★☆ · type U (user space only) · builds on A1-15 (spinlock)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §13 semaphores & condvars, §12 locks & atomics · example: [ex14](../../examples/ex14-libc-function/)

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         sem_init, sem_destroy, sem_wait, sem_trywait, sem_post (semaphore.h - stubs exist)
Description:  A semaphore is a counter (sem_t = unsigned int). sem_init sets it.
              sem_wait waits until the counter is > 0, then decrements it.
              sem_trywait decrements it only if it is > 0 (else -1, no waiting).
              sem_post increments it (and so lets one waiting thread continue).
Return Value: 0, or -1 (NULL; trywait: the counter is 0)
```

## Test

`task start a1-16` → `task test` (`sem_basic.sweb` + `spinlock_basic.sweb` of A1-15).
Done when 7 + 7 checks pass.

## Level 2 – hints

<details><summary>1 – where is the lock?</summary>

A `sem_t` is only the counter – there is no room for a lock inside it. Use **one** static
spinlock in `semaphore.c` for all semaphores: `static pthread_spinlock_t sem_lock = 0;`
</details>

<details><summary>2 – waiting without holding the lock</summary>

`sem_trywait`: lock, if the counter is > 0 decrement it, unlock. `sem_wait`: call `sem_trywait`
in a loop and `sched_yield()` between the tries. Never wait while holding `sem_lock` – then
`sem_post` could never get it.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Builds on A1-15: one global spinlock in `semaphore.c` protects every semaphore's counter.
2. `sem_init(sem, pshared, value)`: NULL → -1; store `value`; return 0. `destroy`: NULL → -1, else 0.
3. `sem_trywait`: lock; counter above 0 → decrease, unlock, return 0; else unlock, return -1.
4. `sem_wait`: loop: lock; counter above 0 → decrease, unlock, return 0; else unlock and `sched_yield()` – **never wait while holding the lock**.
5. `sem_post`: lock; increase; unlock; return 0.
6. `task test`.

</details>

## Your team's code

The stubs are in `userspace/libc/src/semaphore.c`, declared in `semaphore.h`. Your spinlock
from A1-15 is already there (the task starts from its solution).

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Semaphore vs. mutex: what can a semaphore do that a mutex cannot? (count; post from another thread)</summary>

A semaphore **counts**: initialised with n it lets n threads in at once (a mutex: exactly 1). And it has **no owner**: any thread may `sem_post` – e.g. a producer posts, a consumer waits. That makes it good for signalling events and counting resources; a mutex must be unlocked by the thread that locked it.

</details>

<details><summary>Your <code>sem_wait</code> keeps trying – what does a kernel semaphore do instead? (sleep on a wait queue, wake one thread in post: <code>Condition</code> in SWEB's kernel)</summary>

A kernel semaphore lets the waiter **sleep**: `sem_wait` with counter 0 puts the thread into a wait queue (in SWEB's kernel: `Condition::wait` with the semaphore's Mutex), `sem_post` increments and wakes one sleeper (`signal`). The waiter uses no CPU at all while waiting – yours is scheduled again and again just to check.

</details>

<details><summary>Why is one global lock for all semaphores correct? What does it cost?</summary>

Correct, because every semaphore operation (check-and-decrement, increment) happens under one lock – no two operations on any semaphore can interleave. The cost: unrelated semaphores block each other, everything is serialised. A lock per semaphore would need room for it in `sem_t` (here only an `unsigned int`).

</details>
