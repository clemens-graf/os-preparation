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
