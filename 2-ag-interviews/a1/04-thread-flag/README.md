# A1-04 · thread flagging: scheduled twice as often

**Time box 30 min · ★★☆ · builds on A1-01**

**Theory first:** [concepts.pdf](../../concepts.pdf) §11 timer & scheduler, §15 a thread's life · example: [ex13](../../examples/ex13-timer-scheduler/)

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         pthread_setdouble
Nr:           1620
Description:  A flagged thread is scheduled twice as often as normal threads: when it
              would lose the CPU, it keeps it for one more time slice. on = 0 removes the flag.
Parameters:   pthread_t thread (0 = the main thread), int on
Return Value: 0, or -1 if the thread does not exist
```

## Test

`task.sh start a1-04` → `task.sh test`. Two threads spin (no yield) for the same time; done
when, without the flag, both get about the same work (60–160 %), and with the flag the flagged
one gets about twice as much (160–260 %).

## Level 2 – hints

<details><summary>1 – where is the decision made?</summary>

`Scheduler::schedule()` (common/source/kernel/Scheduler.cpp), called from the timer interrupt
(and `yield`). It picks the first schedulable thread of the list and rotates it to the end.
A flagged `currentThread` that has not had its extra slice gets to stay.
</details>

<details><summary>2 – locking - the scheduler cannot lock</summary>

`schedule()` runs with interrupts disabled – a Mutex would assert, a SpinLock could deadlock
the CPU. So the flag the scheduler reads must be one machine word that the syscall writes
atomically (`ArchThreads::atomic_set`), and the "extra slice used" marker is written only by
the scheduler itself.
</details>

<details><summary>3 – finding the thread by id</summary>

Under `process->getThreadLock()`: `findThreadDataByTid(id)`, then its `thread_` – use that
pointer only while holding the lock (`exitThread` clears it under the same lock before the thread dies).
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Add two per-thread fields to `Thread`: the flag ("double slice") and "extra slice already used"; initialise both to 0.
2. Syscall `pthread_setdouble(id, on)` (1620): under `thread_lock_` find the thread object by id (0 = main thread); missing → -1; set the flag (one word); return 0.
3. In `Scheduler::schedule()`, at the very beginning: if the current thread has the flag, has not used its extra slice yet and is still schedulable → mark the extra slice as used, keep it as the current thread (set the register pointer as at the end of `schedule`) and return.
4. Otherwise reset "extra slice used" of the current thread and do the normal round robin.
5. No locks in `schedule()` – interrupts are off.
6. `task test` – the flagged thread does about twice the work.

</details>

## Your team's code

The flag is per thread → `Thread.h`: `volatile size_t double_slice_ = 0; size_t extra_slice_used_ = 0;` (default member initializers – no constructor change). The scheduler change goes into `Scheduler::schedule()` before the round robin. Finding the thread: `findThreadDataByTid(id)->thread_` under `getThreadLock()`. `pthread_setdouble(0, …)` means the calling thread.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why can the scheduler not take a lock? What happens if it tries?</summary>

`schedule()` runs in the timer interrupt (and in `yield`'s interrupt) with **interrupts off**. A Mutex may sleep – there is nobody to switch to inside the scheduler, so SWEB asserts. A SpinLock held by the interrupted thread would never be released: that thread can't run while we spin with interrupts off → deadlock.

</details>

<details><summary>Why is a plain bool enough for the flag – would a 2-word struct be?</summary>

A `bool`/`size_t` is one aligned word: the scheduler reads either the old or the new value, never half of each – both are valid states. A 2-word struct could be read half-updated (one word new, one old) – then you'd need a consistent snapshot, e.g. disable interrupts while writing, or a sequence counter.

</details>

<details><summary>Your test result depends on timing – why is it still reliable?</summary>

The test compares **amounts of work** over many time slices (e.g. 100 vs. 200 units), not exact moments. The scheduler decision itself is deterministic ("one extra slice"); timing noise only shifts a slice or so, which the comparison tolerates.

</details>

<details><summary>How would you implement priorities (n times as often)?</summary>

A counter instead of a flag: `priority_` = n, and in `schedule()` keep the current thread while `slices_used_ < n` (reset when it is switched away). Or put a thread n times into the selection. All without locks – only single words written by the syscall and read by the scheduler.

</details>
