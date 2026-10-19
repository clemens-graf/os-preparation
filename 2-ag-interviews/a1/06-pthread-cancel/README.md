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
`task solution a1-06` is `main` plus the course test. **On `f78faff3` all 9 checks pass.** Until
13 Oct (`40140475`) the test hung at "a thread sleeping in pthread_join is cancelled" – your
`feature/pthread-cancel-points` (merged 18 Oct) fixed exactly that: `cancelThread` now broadcasts
`thread_exited_`, the join loop checks the caller's own cancel flag and returns `JOIN_CANCELED`,
and `Syscall::pthreadJoin` then ends the thread with `exitThread(PTHREAD_CANCELED_VALUE)` – after
the lock is released. The [notes](../../solutions/a1/06-pthread-cancel/NOTES.md) explain the old
bug and why the fix works – a tutor may well ask about both.

Where to look:

| What | Your team's code |
|---|---|
| syscall | `Syscall::pthreadCancel` → `UserProcess::cancelThread(tid)`: sets `requested_cancel_` and broadcasts `thread_exited_`, under `thread_lock_` |
| cancelled while joining | `joinThread`'s wait loop: own `requested_cancel_` → `joined_ = false`, return `JOIN_CANCELED`; `Syscall::pthreadJoin` → `exitThread(PTHREAD_CANCELED_VALUE)` |
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
