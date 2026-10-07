# A1-06 · pthread_cancel (deferred)

**Time box 45 min · ★★★ · builds on A1-01 · your own P2 task**

```
Name:         pthread_cancel
Nr:           1640
Description:  Requests that a thread ends. Deferred cancellation: the thread is not
              killed immediately, it ends at its next cancellation point - every syscall,
              and also while it sleeps in pthread_join. Joining a cancelled thread gives
              PTHREAD_CANCELED.
              A thread that never makes a syscall keeps running.
Parameters:   pthread_t thread
Return Value: 0, or -1 if there is no such running thread (unknown, ended, main thread)
```

## Test

`task.sh start a1-06` → `task.sh test`. Done when all 9 checks pass: a yielding thread, a
thread sleeping in `pthread_join`, and a computing thread that is only cancelled at its next
syscall.

## Hints

<details><summary>1 – why not just kill it?</summary>

The target might be in the middle of a syscall, holding a lock, halfway through changing the
page tables. Killing it from outside leaves all of that broken. Only the thread itself knows
when it is at a safe point: set a flag, let it check.
</details>

<details><summary>2 – where are the cancellation points?</summary>

Top of `Syscall::syscallException` (before any lock is taken) – and in the `pthread_join`
wait loop. A thread sleeping on a Condition must be woken to notice the flag: broadcast after
setting it, and make the wait loop return a "cancelled" result. It then dies in the syscall
layer *after* releasing the lock.
</details>

<details><summary>3 – never die holding a lock</summary>

`kill()` inside `joinThread` while holding `threads_lock_` → every other thread of the process
hangs at the next `pthread_create`. Return out of the locked function first.
</details>

## Questions a tutor might ask

- Why deferred? What could break with asynchronous cancellation?
- Your cancel flag is written by one thread and read by another without a lock – why is that fine?
- A cancelled thread sleeps in pthread_join. Walk me through how it ends.
- What does pthread_setcancelstate(DISABLE) need?
