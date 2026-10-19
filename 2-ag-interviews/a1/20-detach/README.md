# A1-20 · pthread_detach

**Time box 25 min · ★★☆ · type S (syscall + kernel) · study task: your own `pthread_detach`, merged 18 Oct**

**Theory first:** [concepts.pdf](../../concepts.pdf) §15 a thread's life

```
Name:         pthread_detach (POSIX, pthread.h)
Nr:           1504 (sc_pthread_detach - your team's number)
Description:  Marks a thread as detached: nobody will join it, so everything that is kept
              for a join (its exit value, its record) is freed as soon as it ends. If it has
              ended already, free it now. A detached thread cannot be joined.
Parameters:   pthread_t thread
Return Value: 0, or -1 (unknown thread, already detached, or another thread joins it)
```

## Test

`task start a1-20` → `task test` (`detach_basic.sweb`, then `exit`). Done when 6 checks pass.

## Your team's code

**Your team has it – your own `feature/pthread-detach`**, so this is a **study task**: `task start
a1-20` gives you `main`, `task test` runs the 6 course checks against it (all pass). Explain:
`Syscall::pthreadDetach` → `UserProcess::detachThread(tid)` (under `thread_lock_`: refuse unknown,
already detached or `joined_`; an exited thread's record is deleted at once, otherwise
`detached_ = true`), and `exitThread`, which deletes the record of a detached thread instead of
keeping it for a join. A join sees no record → -1.

**To write it again from scratch:** on your task branch, make `detachThread` return -1 and remove
the `detached_` branch in `exitThread`, commit, then rebuild it in the time box.

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
