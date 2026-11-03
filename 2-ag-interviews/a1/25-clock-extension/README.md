# A1-25 · clock extension: CPU time of any thread + time in the timer interrupt

**Time box 40 min · ★★☆ · type S + E (timer interrupt) ([TRIAGE](../../TRIAGE.md)) · from the list: `clock`-extension – *"Track interrupt time · Return runtime of specific thread-ID"***

**Theory first:** [concepts.pdf](../../concepts.pdf) §11 timer & scheduler, §10 interrupts

**This task starts on your team's newer code** (`SubmissionD1`, `f02fd43a`) – the one that already
has `clock_gettime`, `nanosleep` and the `Clock` class. You do **not** build a clock, you **extend
your team's clock** – exactly what a tutor would ask, since it exists already.

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         thread_cputime, timer_irq_time (new, declare them in nonstd.h)
Nr:           1652 (sc_thread_cputime), 1653 (sc_timer_irq_time)
Description:  thread_cputime(thread) returns the CPU time (ns) of ANY running thread of the
              calling process - not only of the caller, which clock_gettime(
              CLOCK_THREAD_CPUTIME_ID) already gives you. A blocked thread's time must not
              grow while other threads run.
              timer_irq_time() returns the total time (ns) the CPU spent in the timer
              interrupt handler since boot.
Parameters:   size_t thread / -
Return Value: ns, or -1 if the thread does not exist or has ended / ns
```

## Test

`task start a1-25` → `task test` (`clock_ext_basic.sweb`, then `exit`). Done when 8 checks pass.
The first `task start` imports your team's commit `f02fd43a` as branch `team/f02fd43a` (read only).

## Level 2 – hints

<details><summary>1 – where your team counts CPU time</summary>

`git grep -n "CountingThreadCycles\|addCpuCycles"`: the scheduler calls
`Clock::stopCountingThreadCycles` for the old thread and `startCountingThreadCycles` for the new one
on every switch – the cycles of a time slice are added to `Thread::cpu_cycles_` (and the process's)
when the slice ends. `Clock::getThreadTime(thread)` returns those cycles **plus "now − start of the
current slice"** – right only for the thread that is running.
</details>

<details><summary>2 – the trap: getThreadTime for another thread</summary>

For a thread that is *not* running, "now − start of the current slice" is the **caller's** running
time – a blocked thread would seem to grow. For another thread use only its counted cycles
(`getCpuCycles()` → `cyclesToNanoseconds`). The test checks exactly this.
</details>

<details><summary>3 – interrupt time</summary>

Read the time-stamp counter (`Cpuid::readTSC()`) at the start of `irqHandler_0`
(`arch/x86/64/source/InterruptUtils.cpp`) and again just **before** `arch_contextSwitch()` – that call
does not come back to the handler. Add the difference to a counter in `Clock`. No lock in an
interrupt handler: interrupts are off, a plain counter is enough. `arch/` does not have
`common/include/misc` on its include path – include `Clock.h`/`Cpuid.h` with a relative path.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Two syscall numbers, two `case`s, two libc wrappers + declarations in `nonstd.h` (add
   `#include "types.h"` for `ssize_t`).
2. `UserProcess::threadCpuTime(tid)`: under `thread_lock_` find the record by tid; no record or
   `thread_ == nullptr` (ended) → -1. If it is the current thread → `Clock::getThreadTime`. Otherwise
   → `cyclesToNanoseconds(thread->getCpuCycles())`.
3. In `Clock`: a counter for the timer interrupt cycles (starts at 0), an `add…` and a `get…` that
   converts to ns.
4. In `irqHandler_0`: TSC at the start; before `arch_contextSwitch()` (and only if the clock is
   initialized) add the difference.
5. The `case` for `timer_irq_time` returns the getter.
6. `task test`.

</details>

## Your team's code

`common/include/misc/Clock.h` / `common/source/misc/Clock.cpp` (`getThreadTime`, `getProcessTime`,
`cyclesToNanoseconds`, `start/stopCountingThreadCycles`), `Cpuid::readTSC`,
`Thread::getCpuCycles`, `Scheduler::schedule` (where the counting happens), `Syscall::clockGetTime`
(the `CLOCK_*` ids), libc `clock()` in `time.c` (= `CLOCK_PROCESS_CPUTIME_ID` in µs).

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why can't you use getThreadTime for another thread?</summary>

It adds the time since the current time slice started – that slice belongs to the *caller*. For a
thread that is not running it would add time it never ran.

</details>

<details><summary>Is the time counted in the timer interrupt part of a thread's CPU time?</summary>

With your team's code: yes – the counting stops in `schedule()`, which runs *inside* the interrupt,
so part of the interrupt is charged to the interrupted thread. `timer_irq_time` makes that time
visible; a variant would subtract it from the thread.

</details>

<details><summary>Why is no lock needed for the interrupt counter?</summary>

The handler runs with interrupts disabled on one CPU – nothing can interrupt the update. The
syscall that reads it reads one aligned 64-bit value, which is atomic on x86-64.

</details>

<details><summary>What is the TSC and why use it instead of ticks?</summary>

The time-stamp counter counts CPU cycles – far finer than timer ticks (milliseconds). Your team's
`Clock` measures its frequency at boot (`cpuid` or via the PIT) to convert cycles to nanoseconds.

</details>
