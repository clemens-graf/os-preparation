# A1-16 · semaphores in user space

**Time box 25 min · ★★☆ · type U (user space only) · builds on A1-15 (spinlock)**

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

## Your team's code

The stubs are in `userspace/libc/src/semaphore.c`, declared in `semaphore.h`. Your spinlock
from A1-15 is already there (the task starts from its solution).

## Hints

<details><summary>1 – where is the lock?</summary>

A `sem_t` is only the counter – there is no room for a lock inside it. Use **one** static
spinlock in `semaphore.c` for all semaphores: `static pthread_spinlock_t sem_lock = 0;`
</details>

<details><summary>2 – waiting without holding the lock</summary>

`sem_trywait`: lock, if the counter is > 0 decrement it, unlock. `sem_wait`: call `sem_trywait`
in a loop and `sched_yield()` between the tries. Never wait while holding `sem_lock` – then
`sem_post` could never get it.
</details>

## Questions a tutor might ask

- Semaphore vs. mutex: what can a semaphore do that a mutex cannot? (count; post from another thread)
- Your `sem_wait` keeps trying – what does a kernel semaphore do instead? (sleep on a wait
  queue, wake one thread in post: `Condition` in SWEB's kernel)
- Why is one global lock for all semaphores correct? What does it cost?
