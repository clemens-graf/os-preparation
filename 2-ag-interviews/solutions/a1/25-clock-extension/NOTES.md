# A1-25 clock extension – solution notes

Patch: [`solution.patch`](solution.patch) – on your team's `SubmissionD1` (`f02fd43a`). Tested: 8 of 8.

```cpp
size_t UserProcess::threadCpuTime(size_t tid)
{
  ScopeLock lock(thread_lock_);
  UserThreadData* data = findThreadDataByTid(tid);
  if (!data || !data->thread_)
    return (size_t) -1;                                       // unknown or ended
  if (data->thread_ == currentThread)
    return Clock::instance()->getThreadTime(currentThread);   // + the running slice
  return Clock::instance()->cyclesToNanoseconds(data->thread_->getCpuCycles());
}

// irqHandler_0
uint64 irq_start = Cpuid::readTSC();
...
Scheduler::instance()->schedule();
if (Clock::isInitialized())
  Clock::instance()->addTimerIrqCycles(Cpuid::readTSC() - irq_start);   // before arch_contextSwitch
```

## Why

- `getThreadTime` adds "now − start of the current slice" – only true for the running thread. The
  test's "blocked thread does not grow" check fails with it (verified: 7 of 8).
- Measure before `arch_contextSwitch()`: it does not return into the handler.
- `arch/` lacks `common/include/misc` on its include path → relative include.

## Shortest working version (tested)

[`short.patch`](short.patch) – **36 lines** instead of 48, passes the same test. Get it with
`task short a1-25` (branch `short/…`, then `task test`).

What is shorter: the interrupt counter is a public field of `Clock` (no add/get methods); `thread_cputime` inline in its `case`.

Use it to see the **core** of the task. In the interview, the full solution's extra checks are what
a tutor asks about ("what if the pointer is invalid / two threads at once?").
