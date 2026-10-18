# A1-20 · pthread_detach

**Time box 25 min · ★★☆ · type S (syscall + kernel) · builds on your team's join**

**Theory first:** [concepts.pdf](../../concepts.pdf) §15 a thread's life

```
Name:         pthread_detach (POSIX, pthread.h - the stub exists)
Nr:           1718 (sc_pthread_detach)
Description:  Marks a thread as detached: nobody will join it, so everything that is kept
              for a join (its exit value, its record) is freed as soon as it ends. If it has
              ended already, free it now. A detached thread cannot be joined.
Parameters:   pthread_t thread
Return Value: 0, or -1 (unknown thread, already detached, or another thread joins it)
```

## Test

`task start a1-20` → `task test` (`detach_basic.sweb`, then `exit`). Done when 6 checks pass.

## Your team's code

What a join needs is your team's `UserThreadData` in `UserProcess::threads_` (`tid_`,
`exit_value_`, `exited_`, `joined_`): it stays after the thread ended until `joinThread` removes
it. `exitThread` frees the stack and sets `exited_`; the libc stub is in `pthread.c`.

## Hints

<details><summary>1 – what has to change?</summary>

A flag `detached_` in `UserThreadData`. `detachThread(tid)` under `thread_lock_`: refuse unknown,
already detached, or `joined_`; if `exited_`: remove and delete the record now; else set the flag.
`exitThread`: if `detached_`, remove and delete the record instead of keeping it.
`joinThread`: a detached thread → -1.
</details>

<details><summary>2 – the race</summary>

"Has it ended?" and "mark it detached" must be one critical section with `exitThread`'s
"mark it ended": otherwise the thread can end between your check and your flag, and its record
is never freed.
</details>

## Questions a tutor might ask

- What exactly is freed for a detached thread, and when? What is left without detach?
- A thread is being joined, and another thread detaches it – what does your code do, and why?
- Why does the detach of an already-ended thread have to free the record itself?
