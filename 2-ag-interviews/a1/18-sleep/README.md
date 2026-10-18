# A1-18 · sleep + uptime

**Time box 25 min · ★★☆ · type S + E (syscall + timer)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §11 timer & scheduler · example: [ex13](../../examples/ex13-timer-scheduler/)

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

## Your team's code

The libc stub is in `userspace/libc/src/unistd.c` (declared in `unistd.h`). The time source
is the timer interrupt: `irqHandler_0` (`InterruptUtils.cpp`) → `Scheduler::incTicks()`;
read it with `Scheduler::instance()->getTicks()`. 1193180 / 65536 = 18.2 ticks per second.

## Hints

<details><summary>1 – kernel or user space?</summary>

User space has no clock (`clock()` is a stub too) – the ticks are in the kernel: a syscall.
</details>

<details><summary>2 – waiting without blocking everybody</summary>

`while (getTicks() < until) Scheduler::instance()->yield();` – every yield lets the other
threads run. Never wait while holding a lock.
</details>

## Questions a tutor might ask

- Your sleeping thread is scheduled again and again just to check the time – how would you
  avoid that? (a per-thread "wake at tick" + state Sleeping; the timer interrupt (or the
  scheduler) wakes it – without taking a lock: interrupts are off there)
- Why may `getTicks()` be read without a lock? (one aligned 64-bit word, only incremented)
- How precise is your sleep? (one tick = 55 ms)
