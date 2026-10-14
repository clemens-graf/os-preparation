# A1-04 · thread flagging: scheduled twice as often

**Time box 30 min · ★★☆ · builds on A1-01**

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

## Your team's code

The flag is per thread → `Thread.h`: `volatile size_t double_slice_ = 0; size_t extra_slice_used_ = 0;` (default member initializers – no constructor change). The scheduler change goes into `Scheduler::schedule()` before the round robin. Finding the thread: `findThreadDataByTid(id)->thread_` under `getThreadLock()`. `pthread_setdouble(0, …)` means the calling thread.

## Hints

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

## Questions a tutor might ask

- Why can the scheduler not take a lock? What happens if it tries?
- Why is a plain bool enough for the flag – would a 2-word struct be?
- Your test result depends on timing – why is it still reliable?
- How would you implement priorities (n times as often)?
