# A1-18 sleep + uptime – solution notes

Patch: [`solution.patch`](solution.patch). Type S + E: a syscall that waits for the timer.

```cpp
size_t Syscall::sleep(size_t seconds)
{
  if (seconds > 100000) seconds = 100000;                       // no overflow
  size_t until = Scheduler::instance()->getTicks() + seconds * 182 / 10;   // 18.2 ticks/s
  while (Scheduler::instance()->getTicks() < until)
    Scheduler::instance()->yield();                             // others run meanwhile
  return 0;
}
// case sc_uptime: return_value = Scheduler::instance()->getTicks();
// libc unistd.c: sleep(s) -> __syscall(sc_sleep, s, ...); nonstd.c: uptime()
```

- Where the time comes from: `irqHandler_0` → `Scheduler::incTicks()` (`ticks_++`) on every timer
  interrupt. Reading `ticks_` without a lock is fine: one aligned word, only ever incremented.
- The yield loop is correct but wasteful: the sleeper is scheduled every round just to see that
  it is not time yet. The real thing: `wake_at_` in the thread + `Scheduler::sleep()`, and the
  scheduler (interrupts off – no locks!) makes a thread with `wake_at_ <= ticks` runnable again.
- Never sleep while holding a Mutex: every other thread that needs it would wait a second too.
