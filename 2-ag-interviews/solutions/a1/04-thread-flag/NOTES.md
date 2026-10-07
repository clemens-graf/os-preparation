# A1-04 thread flagging – solution notes

Patches: [`on-top-of-a1-01.patch`](on-top-of-a1-01.patch), [`solution.patch`](solution.patch).
Also contains the small helper used by A1-04..07: slots remember their `UserThread*`,
`UserProcess::findThread(id)` (caller holds `threads_lock_`).

## The scheduler change

```cpp
// Scheduler::schedule(), before the normal round robin
if (currentThread && currentThread->double_slice_ && !currentThread->extra_slice_used_ &&
    currentThread->schedulable())
{
  currentThread->extra_slice_used_ = 1;          // one extra slice
  currentThreadRegisters = currentThread->switch_to_userspace_ ? ... user_registers_ : kernel_registers_;
  return;                                        // keep running the same thread
}
if (currentThread)
  currentThread->extra_slice_used_ = 0;
```

Result in the test: exactly 100 % vs. 200 % of the work.

## Locking: the scheduler cannot lock

`schedule()` runs in the timer interrupt (and in `yield`'s interrupt) with interrupts **off**. A
Mutex would assert; a SpinLock held by the interrupted thread would never be released → deadlock.
So:
- `double_slice_`: one word, written by the syscall with `ArchThreads::atomic_set`, read by the
  scheduler – it sees either the old or the new value, both fine.
- `extra_slice_used_`: written only by the scheduler itself.
- The syscall looks the thread up under `threads_lock_` and keeps holding it while writing the flag
  (the object cannot be freed meanwhile).

## Variants

Priorities (run n slices), "never schedule this thread" (skip it in the loop), "schedule only
threads of process X for a while".
