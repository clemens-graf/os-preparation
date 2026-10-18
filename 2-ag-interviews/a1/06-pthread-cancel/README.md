# A1-06 · pthread_cancel (deferred)

**Time box 45 min · ★★★ · builds on A1-01 · your own P2 task**

**Theory first:** [concepts.pdf](../../concepts.pdf) §15 a thread's life, §13 semaphores & condvars

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

`task start a1-06` → `task test`: a yielding thread, a thread sleeping in `pthread_join`, and a
computing thread that is only cancelled at its next syscall.

## Your team's code: a study task

Your team already has deferred `pthread_cancel` (your own P2 work, numbers 1502/1503) –
`task solution a1-06` is `main` plus the course test. On `40140475` the first 2 checks pass,
then the test **hangs** at "a thread sleeping in pthread_join is cancelled": `pthread_join` is a
cancellation point in POSIX, but your cancel never wakes a sleeping joiner. The
[notes](../../solutions/a1/06-pthread-cancel/NOTES.md) explain it and show the fix.

Where to look:

| What | Your team's code |
|---|---|
| syscall | `Syscall::pthreadCancel` → `UserProcess::cancelThread(tid)`: sets `requested_cancel_` under `thread_lock_` |
| cancellation point | top of `Syscall::syscallException` (every syscall) and `pthread_testcancel` → `isCancelRequested` |
| ending | the thread calls `exitThread(PTHREAD_CANCELED)` itself – at a safe point, holding no lock |

## Why deferred (keep these in mind)

- Killing a thread from outside could leave it in the middle of a syscall, holding a lock,
  halfway through changing the page tables. Only the thread itself knows a safe point: set a
  flag, let it check.
- A thread sleeping on a Condition must be **woken** to notice the flag (broadcast after setting
  it), and the wait loop must give up when the flag is set – then the thread ends in the
  syscall layer, *after* releasing the lock.
- Never `kill()` while holding `thread_lock_`: every other thread of the process would hang.

## Questions a tutor might ask

- Why deferred? What could break with asynchronous cancellation?
- Your cancel flag is written by one thread and read by another without a lock – why is that fine?
- A cancelled thread sleeps in pthread_join. Walk me through how it ends.
- What does pthread_setcancelstate(DISABLE) need?
