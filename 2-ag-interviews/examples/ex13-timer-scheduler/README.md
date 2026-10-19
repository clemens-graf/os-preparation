# ex13 – the timer interrupt and the scheduler

Two syscalls:
- `timesinfo(&info)` returns two counters of the calling thread: how many timer ticks hit
  while it was running, and how often the scheduler chose it.
- `lowpriority(1)` makes the calling thread run half as often: the scheduler passes it over
  every second time it would choose it.

Test: `ex_timer_scheduler.sweb` (7 checks). A normal and a low-priority thread spin side by
side, and the low-priority one is chosen about half as often (e.g. 16 vs. 31).

Theory: [concepts.pdf](../../concepts.pdf) §11 timer & scheduler, §10 interrupts.

## The three places

| # | File | What |
|---|---|---|
| 1 | `common/include/kernel/Thread.h`, `Thread.cpp` | per-thread fields `ticks_running_`, `times_picked_`, `low_priority_`, `passed_over_` (initialised to 0 in the constructor) |
| 2 | `arch/x86/64/source/InterruptUtils.cpp` → `irqHandler_0` | **every timer tick:** `++currentThread->ticks_running_` – before `schedule()`, so it counts the thread the tick interrupted |
| 3 | `common/source/kernel/Scheduler.cpp` → `schedule()` | **every switch:** the loop that picks the next thread – skip a low-priority thread every second time; `++currentThread->times_picked_` after the choice |

Plus the usual six places of the two syscalls (as in ex01).

The exact, complete change: `example.patch` in this folder, or `task example ex13` and then
`git -C ~/Documents/University/2026-fall/os/repos/sweb-ag show` (`q` quits).

## How the timer reaches the scheduler

```
PIT (18.2 Hz) → IRQ 0 → arch_irqHandler_0 (asm, saves registers)
  → irqHandler_0()                 InterruptUtils.cpp
      incTicks()                   the global tick counter (ex01: getticks)
      ++currentThread->...         <- per-thread counting goes here
      schedule()                   picks the next thread: first schedulable one, then rotate the list
      arch_contextSwitch()         loads the registers of the new currentThread
sched_yield() → int 65 → irqHandler_65 → schedule() too   (that is why every yield counts as a pick)
```

## Why exactly like this

- **No locks, no `new`/`delete`, no `kprintf` in `irqHandler_0` or `schedule()`:** interrupts are
  off. If a lock is taken, the handler would have to sleep, and nothing could wake it. A plain
  `++` on a `size_t` field is safe here, because nobody can interrupt it.
- **Single words, no lock in the syscall either:** the syscall only *reads* the counters, and only
  the thread itself writes `low_priority_`. An aligned 8-byte read or write is atomic on x86-64.
- **`if (currentThread)` in the timer handler:** the first tick comes before the first
  `schedule()`. Without the check the kernel writes through a null pointer and does not boot.
  This happened while this example was being written.
- **Skipping is safe because the idle thread is always schedulable.** If you change the
  loop so that it can skip *every* thread, `assert(it != threads_.end())` fires.
- **Kernel struct = user struct, written twice:** the kernel cannot include libc headers, so
  `TimesInfo` exists in `Syscall.cpp` and in `nonstd.h` with the same fields in the same order.

## Tasks that use this

| Task | What is the same | What is different |
|---|---|---|
| A1-07 thread runtime | the tick counter per thread in `irqHandler_0` | other threads by id; the whole process, also threads that have ended |
| A1-04 thread flag | a flag the scheduler reads | run *twice* as often: keep the CPU one more slice |
| A1-05 pthread_invoke | the choice in `schedule()` | choose a *given* thread next |
| A1-18 sleep | ticks + "not schedulable yet" | wake-up tick per thread, `schedulable()` |
