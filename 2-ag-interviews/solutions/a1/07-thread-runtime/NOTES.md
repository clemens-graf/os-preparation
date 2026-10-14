# A1-07 thread runtime + clock() – solution notes

Patch: [`solution.patch`](solution.patch) – on your team's code.

```cpp
void Scheduler::incTicks()          // timer interrupt, interrupts off
{
  if (currentThread)
    ++currentThread->cpu_ticks_;    // the interrupted thread used this tick
  ++ticks_;
}
```

- `thread_runtime(id)`: under `thread_lock_`, `findThreadDataByTid(id)->thread_->cpu_ticks_`.
- Process time: main thread + running threads + `finished_ticks_` (added in `exitThread`,
  when the destructor still has the counter).
- `clock()` in `userspace/libc/src/time.c`: `ticks * 54925` µs (1193180 / 65536 = 18.2 Hz).

## Locking

The counter is written only by the timer interrupt (no lock possible there) and read by syscalls:
a single 64-bit word – atomic on x86-64. On 32-bit SWEB a 64-bit counter could tear (two halves).

## Accuracy

Tick sampling: a thread that always yields just before the tick is never charged (the lazy
thread: 0 ticks). Real kernels measure with the time stamp counter on every switch.
