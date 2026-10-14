# A1-06 pthread_cancel – study notes (your team's code)

Patch: [`solution.patch`](solution.patch) is **empty** – your team's `main` already has deferred
`pthread_cancel` (your own P2 work). `task solution a1-06` = `main` + the course test.

## How your team's cancel works

```cpp
size_t UserProcess::cancelThread(size_t tid)          // the canceller
{
  ScopeLock lock(thread_lock_);
  UserThreadData* thread_data = findThreadDataByTid(tid);
  if (!thread_data || thread_data->exited_) return -1;
  thread_data->requested_cancel_ = true;              // only a flag - the target ends itself
  return 0;
}

// the cancellation point: top of Syscall::syscallException, before any lock is taken
if (process && process->isCancelRequested(currentThread->getTID()))
  process->exitThread(PTHREAD_CANCELED_VALUE);        // joiners get PTHREAD_CANCELED
```

`pthread_testcancel` is an empty syscall: the check at the top of `syscallException` does the work.

## What the course test shows: 2 pass, then it hangs

Check 3: thread J sleeps in `pthread_join` (in `thread_exited_.wait()`), main cancels J and joins it.

- `cancelThread` sets J's flag – but **nobody wakes J**.
- And even when J wakes up (another thread exits), its wait loop only checks `exited_`, never
  the cancel flag.

So J sleeps until the thread it waits for ends – in the test never, so main waits for J
forever. POSIX lists `pthread_join` as a cancellation point: a cancelled joiner must end.

**Fix direction:**

```cpp
// cancelThread: after setting the flag, wake every sleeper of this process
thread_data->requested_cancel_ = true;
thread_exited_.broadcast();

// joinThread: give up when the CALLER was cancelled (we hold thread_lock_ already -
// isCancelRequested() would take it again: Mutexes are not recursive -> look it up directly)
while (!thread_data->exited_)
{
  UserThreadData* self = findThreadDataByTid(currentThread->getTID());
  if (self && self->requested_cancel_)
  {
    thread_data->joined_ = false;        // the target can be joined by someone else later
    return JOIN_CANCELLED;               // a new return value, e.g. (size_t) -2
  }
  thread_exited_.wait();
}

// Syscall::pthreadJoin: the lock is released now - end at this safe point
if (result == JOIN_CANCELLED)
  process->exitThread(PTHREAD_CANCELED_VALUE);
```

The order matters: the thread must not end *inside* `joinThread` – it holds `thread_lock_` there.
Return first, then `exitThread` in the syscall layer.

## Why deferred

- Killing a thread from outside could catch it in the middle of a syscall: holding
  `thread_lock_`, halfway through `mapPage`. Everything it was doing stays broken.
- Only the thread itself knows a safe point – so: set a flag, the thread checks it.
- The flag is written under `thread_lock_` and read under it (`isCancelRequested`) – no race.

## Questions a tutor might ask

- Why deferred? What could break with asynchronous cancellation?
- A cancelled thread sleeps in `pthread_join`. Walk me through how it ends (now: why it does not).
- A thread computes without syscalls – when does it end after `pthread_cancel`?
- What would `pthread_setcancelstate(PTHREAD_CANCEL_DISABLE)` need? (a second flag in
  `UserThreadData`, checked at the same places)
