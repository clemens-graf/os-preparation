# A1-18 · sleep + uptime

**Time box 25 min · ★★☆ · type S + E (syscall + timer)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §11 timer & scheduler · example: [ex13](../../examples/ex13-timer-scheduler/)

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         sleep (POSIX, unistd.h - the stub exists), uptime
Nr:           1715 (sc_sleep), 1716 (sc_uptime)
Description:  sleep(seconds) returns after (about) seconds seconds. While a thread sleeps,
              the other threads (of all processes) keep running. sleep(0) returns at once.
              uptime() returns the timer ticks since boot (18.2 per second).
Return Value: sleep: 0 (seconds not slept); uptime: ticks
```

## Test

`task start a1-18` → `task test` (`sleep_basic.sweb`, then `exit`). Done when 5 checks pass.

## Level 2 – hints

<details><summary>1 – kernel or user space?</summary>

User space has no clock (`clock()` is a stub too) – the ticks are in the kernel: a syscall.
</details>

<details><summary>2 – waiting without blocking everybody</summary>

`while (getTicks() < until) Scheduler::instance()->yield();` – every yield lets the other
threads run. Never wait while holding a lock.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Syscall plumbing for `sleep` (1715 – the libc stub in `unistd.c` exists) and `uptime` (1716, new wrapper).
2. `uptime`: return the scheduler's tick counter (`getTicks()`) – no lock needed.
3. `sleep(seconds)`: compute the end tick = now + seconds × 18.2 (use integers: × 182 / 10); cap huge values so nothing overflows.
4. Loop: while the tick counter is below the end tick, **yield** – the other threads run meanwhile.
5. Return 0. `sleep(0)` must return at once (the loop does not run).
6. Make sure you hold no lock while sleeping.
7. `task test`. (Better design for the interview: a wake-up tick per thread + state Sleeping, woken by the scheduler – explain it, the yield loop is enough to pass.)

</details>

## Your team's code

The libc stub is in `userspace/libc/src/unistd.c` (declared in `unistd.h`). The time source
is the timer interrupt: `irqHandler_0` (`InterruptUtils.cpp`) → `Scheduler::incTicks()`;
read it with `Scheduler::instance()->getTicks()`. 1193180 / 65536 = 18.2 ticks per second.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Your sleeping thread is scheduled again and again just to check the time – how would you avoid that? (a per-thread "wake at tick" + state Sleeping; the timer interrupt (or the scheduler) wakes it – without taking a lock: interrupts are off there)</summary>

Give the thread a wake-up time and a state: `wake_at_ = now + ticks`, state Sleeping (not schedulable), then yield. On every tick the scheduler (or the timer interrupt) checks the sleeping threads and makes those with `wake_at_ <= ticks` runnable again. **No locks** there – interrupts are off – so only single words, written by the thread itself before it sleeps.

</details>

<details><summary>Why may <code>getTicks()</code> be read without a lock? (one aligned 64-bit word, only incremented)</summary>

`ticks_` is one aligned 64-bit word, written only by the timer interrupt (`incTicks`) and only ever incremented. A read returns the old or the new value, never a mix (atomic on x86-64) – both are valid answers to "what time is it".

</details>

<details><summary>How precise is your sleep? (one tick = 55 ms)</summary>

At best one tick, ≈ 55 ms (18.2 per second): `sleep(1)` waits 18 ticks ≈ 0.99 s plus whatever part of the current tick had already passed, plus the time until the scheduler picks the thread again. Sub-tick precision needs a faster timer or a high-resolution clock (`rdtsc`).

</details>
