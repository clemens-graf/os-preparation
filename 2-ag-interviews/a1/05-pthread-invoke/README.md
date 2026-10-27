# A1-05 · pthread_invoke: run a specific thread next

**Time box 30 min · ★★☆ · type E (scheduler) + S ([TRIAGE](../../TRIAGE.md)) · builds on A1-01**

**Theory first:** [concepts.pdf](../../concepts.pdf) §11 timer & scheduler, §15 a thread's life · example: [ex13](../../examples/ex13-timer-scheduler/)

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         pthread_invoke
Nr:           1630
Description:  The calling thread gives up the CPU, and the given thread of the same
              process runs next (a directed yield) - if it is runnable.
Parameters:   pthread_t thread
Return Value: 0, or -1 if the thread does not exist or is the caller
```

## Test

`task.sh start a1-05` → `task.sh test`. Five threads loop with `sched_yield`; done when the
target runs first after `pthread_invoke` in at least 18 of 20 tries (with a plain `sched_yield`
it is about 4/20).

## Level 2 – hints

<details><summary>1 – how to tell the scheduler?</summary>

A `Thread* preferred_` in the scheduler, set before yielding; `schedule()` takes it (and clears
it) if that thread is in its list and schedulable, else normal round robin.
</details>

<details><summary>2 – the dangling pointer</summary>

Between your syscall and the next `schedule()` the target may die and be deleted. The scheduler
must never dereference `preferred_` blindly: compare it with the pointers in its own list first –
only list members are alive.
</details>

<details><summary>3 – no lock while yielding</summary>

Look the thread up under the process' thread lock, release the lock, then yield.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Add a "preferred next thread" pointer to the `Scheduler` and a method that sets it and yields.
2. Syscall `pthread_invoke(id)` (1630): under `thread_lock_` find the target's thread object; missing or the caller itself → -1; remember the pointer; **release the lock**, then call the method (never yield holding a lock).
3. In `schedule()`: take the preferred pointer and clear it. Walk the thread list: if you find **that pointer** in the list and it is schedulable, choose it.
4. If not found (ended meanwhile, sleeping): normal round robin.
5. Compare the pointer only – never dereference it before you found it in the list.
6. `task test`.

</details>

## Your team's code

`Scheduler.h`: `Thread* volatile preferred_ = nullptr;` + `yieldTo(thread)`; `Scheduler::schedule()` looks for `preferred_` first. The syscall finds the target with `findThreadDataByTid(id)->thread_` under `getThreadLock()` and calls `yieldTo` **after** releasing the lock.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why may <code>schedule()</code> compare the pointer but not dereference it before finding it in the list?</summary>

After `thread_lock_` is released, the target may end and be **deleted** before `schedule()` runs – `preferred_` may dangle. Comparing a pointer value is harmless; dereferencing it is a use-after-free. Every thread in the scheduler's own list is alive (the CleanupThread removes threads from the list before deleting them), so `schedule()` touches it only after finding it there.

</details>

<details><summary>What if the target thread is sleeping (blocked in join)?</summary>

It's not `schedulable()` (state Sleeping), so `schedule()` doesn't find a runnable match and falls back to the normal round robin. `pthread_invoke` is only a hint; it can't make a blocked thread run – that would break whatever it waits for.

</details>

<details><summary>How would <code>pthread_switch(tid)</code> (caller sleeps until switched back to) differ?</summary>

The caller would also give up its *right to run*: it becomes non-schedulable until someone switches back to it (or the target ends) – a true hand-over like coroutines. You'd need a per-thread "waiting for switch-back" state, a way to wake it, and an answer to "what if the target never switches back". `pthread_invoke` only reorders: the caller stays runnable. (The register-level switch itself is A1-22.)

</details>
