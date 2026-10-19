# A1-07 · thread runtime + clock()

**Time box 30 min · ★★☆ · builds on A1-01**

**Theory first:** [concepts.pdf](../../concepts.pdf) §11 timer & scheduler, §15 a thread's life · example: [ex13](../../examples/ex13-timer-scheduler/)

```
Name:         thread_runtime, clock
Nr:           1650 (sc_thread_runtime), 1651 (sc_process_ticks)
Description:  thread_runtime(thread) returns the timer ticks the thread was running
              (0 = main thread). clock() (POSIX, time.h) returns the processor time of the
              whole process - all threads, also the ones that have ended - in
              CLOCKS_PER_SEC units (microseconds).
Parameters:   pthread_t thread / -
Return Value: ticks, or -1 if the thread does not exist / clock_t
Notes:        The timer runs at 1193180 / 65536 = 18.2 Hz: one tick = 54925 us.
```

## Test

`task.sh start a1-07` → `task.sh test`. Done when 5 checks pass: a busy thread is charged
many ticks, a yielding one almost none, `clock()` includes ended threads.

## Your team's code

Per thread → `Thread.h`: `volatile size_t cpu_ticks_ = 0;`, incremented in `Scheduler::incTicks()` (called from the timer interrupt). Per process → `UserProcess`: `finished_ticks_` (added in `exitThread`) + the running threads in `threads_`. `thread_runtime(0)` = the calling thread. `clock()` in `userspace/libc/src/time.c` is a stub – it calls the new syscall.

## Hints

<details><summary>1 – where to count?</summary>

The timer interrupt calls `Scheduler::incTicks()` while `currentThread` is the interrupted thread
– charge the tick to it there. Interrupts are off: no lock; one 64-bit counter that only the
timer writes is fine.
</details>

<details><summary>2 – ended threads</summary>

Their objects are gone. Add their ticks to a per-process total in `exitThread` (the
destructor still has the counter).
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why is counting in the timer interrupt only an approximation?</summary>

The tick is counted for whichever thread was running **when the timer fired** – a thread that ran 54 ms and yielded just before the tick gets 0, the next one gets the full tick. Over many ticks it averages out, but short-running threads are under- or overcounted. Exact would need a time stamp (e.g. `rdtsc`) at every switch.

</details>

<details><summary>Reading a 64-bit counter without a lock while the timer writes it – safe on x86-64? On 32-bit?</summary>

On x86-64 an aligned 8-byte load or store is atomic: you read the old or the new value, never a mix. On 32-bit x86 a 64-bit counter takes two 32-bit accesses – you could read the low half after it wrapped and the high half before the carry, a value off by 2³². There you'd need a lock with interrupts off, or read high-low-high until it's consistent.

</details>

<details><summary>Why does SWEB's timer run at 18.2 Hz, and what does that mean for <code>sched_yield</code>?</summary>

It's the PIT's default: base frequency 1193180 Hz divided by 65536 (divisor 0) ≈ 18.2 Hz, i.e. one tick ≈ 55 ms, and SWEB's main sets `IRQ0_TIMER_FREQUENCY` 0. For `sched_yield`: a thread that yields gives up the rest of its slice at once; a busy thread without yield runs a full 55 ms before anyone else gets the CPU.

</details>
