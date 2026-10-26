# A1-17 · condition variables in user space

**Time box 25 min · ★★☆ · type U (user space only) · builds on A1-15 (mutex)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §13 semaphores & condvars, §12 locks & atomics · example: [ex14](../../examples/ex14-libc-function/)

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

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

## Level 2 – hints

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

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Builds on A1-15. A condition variable is a counter ("generation").
2. `init`: NULL → -1; set to 0. `destroy`: NULL → -1, else 0.
3. `pthread_cond_wait(cond, mutex)`: read the counter **while still holding the mutex**; unlock the mutex; yield in a loop until the counter is different; lock the mutex again; return 0.
4. Read the counter through a `volatile` pointer, so the loop really reads memory each time.
5. `signal` and `broadcast`: increase the counter **atomically** (`__sync_fetch_and_add`); return 0.
6. `task test` – the producer/consumer check fails if you read the counter after unlocking (lost wake-up).

</details>

## Your team's code

The stubs are in `userspace/libc/src/pthread.c` (declared in `pthread.h`);
`pthread_cond_t` is an `unsigned int`. The task starts from the A1-15 solution: your
`pthread_mutex_*` work.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why must <code>pthread_cond_wait</code> always be called in a <code>while (condition)</code> loop?</summary>

Because waking up doesn't mean the condition is true: another thread may have been faster and taken the item, `broadcast` wakes all waiters but only one can use it, and spurious wake-ups are allowed (yours wakes everyone on every signal). The `while` re-checks the condition with the mutex held and waits again if needed.

</details>

<details><summary>Why does wait take the mutex as a parameter? What would go wrong without it (lost wake-up)?</summary>

The waiter checks the condition while holding the mutex, then must release it so the signaller can change the state. Releasing and starting to wait must look **atomic**: if the signaller could get the mutex, change the state and signal *between* "unlock" and "wait", the signal is lost and the waiter sleeps forever. Your version reads the generation **before** unlocking – a signal after that changes the number, so it is noticed.

</details>

<details><summary>How would the kernel version look? (a wait queue: <code>Condition::wait</code> / <code>signal</code> – the waiter sleeps instead of yielding; your team's <code>thread_exited_</code> is one)</summary>

A wait queue in the kernel: `pthread_cond_wait` → syscall → `Condition::wait()`, which puts the thread to sleep **and** releases the Mutex in one step; `signal` takes one sleeper off the queue and makes it runnable, `broadcast` all. The waiter costs no CPU. Your team's `thread_exited_` in `joinThread` is exactly this.

</details>
