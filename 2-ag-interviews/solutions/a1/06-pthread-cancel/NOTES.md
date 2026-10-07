# A1-06 pthread_cancel – solution notes

Patches: [`on-top-of-a1-01.patch`](on-top-of-a1-01.patch), [`solution.patch`](solution.patch).

## Design: deferred cancellation

```cpp
size_t Syscall::pthreadCancel(size_t thread_id)
{
  ScopeLock lock(process->getThreadsLock());
  Thread* target = process->findThread(thread_id);
  if (!target || target == process) return -1;
  ArchThreads::atomic_set(((UserThread*) target)->cancel_requested_, 1);
  process->wakeJoiners();                  // broadcast: it may sleep in pthread_join
  return 0;
}

void Syscall::cancelIfRequested()          // at cancellation points, NO lock held
{
  if (/* user thread, not main */ && self->cancel_requested_) {
    self->exit_value_ = (size_t) -1;       // PTHREAD_CANCELED
    currentThread->kill();
  }
}
```

Cancellation points:
1. top of `syscallException` – before any lock is taken;
2. the `pthread_join` wait loop – `joinThread` returns `JOIN_CANCELED`, `Syscall::pthreadJoin`
   calls `cancelIfRequested()` **after** `joinThread`'s ScopeLock released the mutex.

## Why not kill the target directly?

It may be in the middle of a syscall: holding `arch_memory_lock_`, half-way through `mapPage`,
in the middle of a disk request. Killing it there leaves a lock held forever and data half
changed. Only the thread knows its safe points.

## Locking

- The flag: one word, written atomically, read by the target without a lock (it only checks it;
  a stale read just means it is cancelled at the next point).
- `findThread` + flag write under `threads_lock_`: the thread object cannot be freed meanwhile.
- **Never `kill()` while holding a lock** – `~Thread` asserts, and every other thread would block.

## Test

A yielding thread, a thread sleeping in `pthread_join` (woken by the broadcast), a computing
thread that is only cancelled at its next syscall (the code after it never runs), unknown/ended
threads → -1.

## For your P2 work

`pthread_setcancelstate`/`setcanceltype` (electives): state = "ignore requests for now" (check it
at the points), type asynchronous = kill at the next *timer interrupt* in user mode – only safe if
the thread is in user mode (not in a syscall).
