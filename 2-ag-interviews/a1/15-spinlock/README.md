# A1-15 · pthread spinlock + mutex in user space

**Time box 25 min · ★★☆ · type U (user space only) · builds on A1-01 (pthread_join)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §12 locks & atomics · example: [ex14](../../examples/ex14-libc-function/)

```
Name:         pthread_spin_init/destroy/lock/trylock/unlock,
              pthread_mutex_init/destroy/lock/unlock (pthread.h - the stubs exist)
Description:  A lock is one unsigned int (0 = free, 1 = held). lock waits until the lock
              is free and takes it; trylock takes it only if it is free; unlock frees it.
              While waiting, give the CPU to the other threads (sched_yield) - the holder
              needs it to unlock. The mutex may behave exactly like the spinlock.
Return Value: 0, or -1 (NULL; trylock: the lock is held)
```

## Test

`task start a1-15` → `task test` (`spinlock_basic.sweb`: 4 threads × 20000 increments under
each lock). Done when 7 checks pass.

## Your team's code

The stubs are in `userspace/libc/src/pthread.c`. **Trap:** `pthread_spin_*` exist there but
are **not declared** in `userspace/libc/include/pthread.h` – a test that calls them does not
compile until you add the five declarations. `pthread_mutex_*` are declared.
Your team's `pthread_create`/`pthread_join` work, so the test can use threads.


## Hints

<details><summary>1 – why not just "if (*lock == 0) *lock = 1;"?</summary>

Between reading `0` and writing `1` the timer can switch to another thread, which also reads
`0` – both think they own the lock. Check and set must be **one** step. The CPU has an
instruction for that (`xchg`); GCC gives it to you as a builtin, no include needed:

```c
__sync_lock_test_and_set(lock, 1)   // writes 1, returns the OLD value - in one step
__sync_lock_release(lock)           // writes 0
```
Old value 0 → the lock was free and is yours now. Old value 1 → someone holds it: yield, retry.
</details>

<details><summary>2 – the mutex</summary>

`pthread_mutex_t` is also an `unsigned int`: let the mutex functions call the spinlock
functions. (A real mutex would sleep in the kernel instead of yielding in a loop - with a
syscall and a wait queue.)
</details>

## Honest note about the test

A lock **without** the atomic step ("check, then set") usually **passes** this test too: the
gap between check and set is a few instructions, and the timer rarely hits exactly there. That
is why such bugs survive testing – and why the tutor will ask *why* your version is correct,
not whether the test passed. Know the answer from hint 1.

## Questions a tutor might ask

- Why does a lock need an atomic instruction, while A0-13's `atexit` counter does not?
- What is the difference between a spinlock and a mutex? When is spinning OK?
- Why `sched_yield` in the loop? What happens on one CPU without it?
- What happens if a thread unlocks a lock it does not hold? If it locks twice?
