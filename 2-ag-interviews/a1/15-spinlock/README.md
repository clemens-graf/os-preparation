# A1-15 · pthread spinlock + mutex in user space

**Time box 25 min · ★★☆ · type U (user space only) · builds on A1-01 (pthread_join)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §12 locks & atomics · example: [ex14](../../examples/ex14-libc-function/)

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

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

## Level 2 – hints

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

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Declare the missing functions in `pthread.h` if the test does not compile.
2. `pthread_spin_init` / `pthread_mutex_init`: NULL → -1; set the lock to 0; return 0. `destroy`: NULL → -1, else 0.
3. `lock`: NULL → -1. Loop: atomically write 1 and get the old value (`__sync_lock_test_and_set`); while the old value was 1, `sched_yield()` and try again.
4. `trylock`: one atomic attempt; old value 1 → return -1 (held), else 0.
5. `unlock`: NULL → -1; set the lock back to 0 (`__sync_lock_release`); return 0.
6. The mutex functions may simply call the spinlock functions.
7. `task test` – the test counts with many threads; a non-atomic lock loses increments.

</details>

## Honest note about the test

A lock **without** the atomic step ("check, then set") usually **passes** this test too: the
gap between check and set is a few instructions, and the timer rarely hits exactly there. That
is why such bugs survive testing – and why the tutor will ask *why* your version is correct,
not whether the test passed. Know the answer from hint 1.

## Your team's code

The stubs are in `userspace/libc/src/pthread.c`. **Trap:** `pthread_spin_*` exist there but
are **not declared** in `userspace/libc/include/pthread.h` – a test that calls them does not
compile until you add the five declarations. `pthread_mutex_*` are declared.
Your team's `pthread_create`/`pthread_join` work, so the test can use threads.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why does a lock need an atomic instruction, while A0-13's <code>atexit</code> counter does not?</summary>

A lock's *whole job* is that two threads trying **at the same moment** can't both get it – checking "free" and writing "taken" must be one indivisible step (`xchg`), else the timer can switch between them and both enter. `atexit` is called by one thread at startup; two threads registering at the very same moment is unusual – a plain counter is acceptable there (and you can say so).

</details>

<details><summary>What is the difference between a spinlock and a mutex? When is spinning OK?</summary>

A spinlock **waits actively** – it keeps trying (here: yields in a loop). A mutex puts the waiting thread **to sleep** and the unlocker wakes it – that needs the kernel (a wait queue). Spinning is OK when the wait is very short and the holder runs on another CPU, or when sleeping is impossible (interrupt handlers, inside the scheduler).

</details>

<details><summary>Why <code>sched_yield</code> in the loop? What happens on one CPU without it?</summary>

On one CPU the holder can only release the lock when it **runs**. Without `sched_yield` the waiter spins for its whole 55 ms time slice, achieving nothing – every attempt fails. With `sched_yield` it gives the CPU away at once, the holder runs, unlocks, and the next attempt succeeds.

</details>

<details><summary>What happens if a thread unlocks a lock it does not hold? If it locks twice?</summary>

Unlock of a lock you don't hold: your version just writes 0 – it frees somebody else's lock, and two threads can be inside. Locking twice from the same thread: it spins forever on its own lock (self-deadlock). POSIX error-checking mutexes store the owner's thread id to detect both; recursive mutexes count instead.

</details>
