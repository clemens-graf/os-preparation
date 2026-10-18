# A1-17 · condition variables in user space

**Time box 25 min · ★★☆ · type U (user space only) · builds on A1-15 (mutex)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §13 semaphores & condvars, §12 locks & atomics · example: [ex14](../../examples/ex14-libc-function/)

```
Name:         pthread_cond_init, pthread_cond_destroy, pthread_cond_wait,
              pthread_cond_signal, pthread_cond_broadcast (pthread.h - the stubs exist)
Description:  pthread_cond_wait(cond, mutex): the caller holds mutex. Release it, wait until
              the condition is signalled, take the mutex again, return. signal wakes at
              least one waiter, broadcast all of them (waking more than asked is allowed:
              POSIX permits spurious wake-ups - callers always wait in a loop).
              A signal that happens after a thread released the mutex in wait must not be lost.
Return Value: 0, or -1 (NULL)
```

## Test

`task start a1-17` → `task test` (`cond_basic.sweb` + A1-15's `spinlock_basic.sweb`).
Done when 5 + 7 checks pass. If `cond_wait` keeps the mutex, the test hangs.

## Your team's code

The stubs are in `userspace/libc/src/pthread.c` (declared in `pthread.h`);
`pthread_cond_t` is an `unsigned int`. The task starts from the A1-15 solution: your
`pthread_mutex_*` work.

## Hints

<details><summary>1 – what can one unsigned int remember?</summary>

A counter of signals – a "generation". `signal` increments it; `wait` waits until it is no
longer the value it had when the thread started waiting.
</details>

<details><summary>2 – the lost wake-up</summary>

Read the generation **before** you release the mutex. Then a signal that comes between the
unlock and your first check still changes the number you compare with.
</details>

<details><summary>3 – waiting</summary>

`while (generation unchanged) sched_yield();` – then `pthread_mutex_lock` again. The increment in
`signal` must be atomic (`__sync_fetch_and_add`): two signallers at once must not lose one.
</details>

## Questions a tutor might ask

- Why must `pthread_cond_wait` always be called in a `while (condition)` loop?
- Why does wait take the mutex as a parameter? What would go wrong without it (lost wake-up)?
- How would the kernel version look? (a wait queue: `Condition::wait` / `signal` – the waiter
  sleeps instead of yielding; your team's `thread_exited_` is one)
