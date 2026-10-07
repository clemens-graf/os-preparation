# A1-07 · thread runtime + clock()

**Time box 30 min · ★★☆ · builds on A1-01**

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

## Hints

<details><summary>1 – where to count?</summary>

The timer interrupt calls `Scheduler::incTicks()` while `currentThread` is the interrupted thread
– charge the tick to it there. Interrupts are off: no lock; one 64-bit counter that only the
timer writes is fine.
</details>

<details><summary>2 – ended threads</summary>

Their objects are gone. Add their ticks to a per-process total in `threadFinished` (the
destructor still has the counter).
</details>

## Questions a tutor might ask

- Why is counting in the timer interrupt only an approximation?
- Reading a 64-bit counter without a lock while the timer writes it – safe on x86-64? On 32-bit?
- Why does SWEB's timer run at 18.2 Hz, and what does that mean for `sched_yield`?
