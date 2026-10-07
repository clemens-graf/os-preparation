# A1-05 · pthread_invoke: run a specific thread next

**Time box 30 min · ★★☆ · builds on A1-01**

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

## Hints

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

## Questions a tutor might ask

- Why may `schedule()` compare the pointer but not dereference it before finding it in the list?
- What if the target thread is sleeping (blocked in join)?
- How would `pthread_switch(tid)` (caller sleeps until switched back to) differ?
