# A1-03 · pthread barriers

**Time box 40 min · ★★★ · type S ([TRIAGE](../../TRIAGE.md)) · builds on A1-01 · man page: pthread_barrier_wait**

**Theory first:** [concepts.pdf](../../concepts.pdf) §14 barriers, §13 semaphores & condvars

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         pthread_barrier_init, pthread_barrier_wait, pthread_barrier_destroy
Nr:           1610, 1611, 1612
Description:  A barrier for count threads: pthread_barrier_wait blocks until count threads
              have called it, then all of them continue. Exactly one of them gets
              PTHREAD_BARRIER_SERIAL_THREAD, the others 0. Afterwards the barrier is
              reusable for the next round.
              destroy fails while threads are waiting.
Parameters:   pthread_barrier_t* barrier, const pthread_barrierattr_t* attr (ignored),
              unsigned count (> 0)
Return Value: init/destroy: 0 or -1; wait: 1 (serial), 0, or -1 (invalid barrier)
Notes:        SWEB convention: -1 means error, so PTHREAD_BARRIER_SERIAL_THREAD is 1 here
              (Linux: -1). pthread_barrier_t holds the id of a kernel barrier.
```

## Test

`task.sh start a1-03` → `task.sh test` (3 runs). Done when all 6 checks pass in every run:
4 threads, 20 rounds, nobody passes early, exactly one serial thread per round.

## Level 2 – hints

<details><summary>1 – what does the kernel keep per barrier?</summary>

`count`, `waiting` (arrived in this round), and a **generation** counter. A small table in
the process, one Mutex for it, one Condition (broadcast when any barrier opens).
</details>

<details><summary>2 – the classic bug</summary>

Waiting for `waiting == 0` (or `waiting == count`): when the last thread arrives it resets
`waiting = 0` – but a fast thread may already arrive for the NEXT round before the sleepers
wake up, so `waiting` is 1 again and they sleep forever. Wait for "the generation changed"
instead: remember it before sleeping, loop while it is the same.
</details>

<details><summary>3 – signal or broadcast?</summary>

Broadcast: all waiters of the round must wake. With one Condition for all barriers, waiters
of other barriers wake too – the generation check sends them back to sleep.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. In `UserProcess`: an array of barriers (used, count, waiting, generation), one Mutex for them and one Condition.
2. `pthread_barrier_init(barrier, attr, count)`: count 0 → -1; under the lock take a free slot, store count; write its id (index + 1) into the user's barrier variable.
3. `pthread_barrier_wait`: under the lock find the barrier; remember its current generation; increase `waiting`.
4. If `waiting` equals `count`: reset `waiting` to 0, increase the generation, broadcast, return the serial value (1).
5. Otherwise: wait on the Condition **while the generation is still the remembered one**; return 0.
6. `destroy`: free the slot. `task test`.

</details>

## Your team's code

Barriers are per process → `UserProcess`: `Barrier barriers_[16]`, their own `barriers_lock_` (Mutex) and `barrier_opened_` (Condition), initialised in the constructor's initializer list **after** the existing members (the order must match the declaration order). Syscalls 1610–1612 call `getProcess()->barrierInit/Wait/Destroy`.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why the generation counter? Draw the round where <code>waiting == 0</code> breaks.</summary>

With `while (waiting != 0) wait();`: n = 2, round 1: A waits (waiting = 1). B arrives, sets `waiting = 0`, broadcasts, returns – and runs on to the barrier of **round 2**: `waiting = 1`, sleeps. Now A wakes, checks `waiting != 0` → true, sleeps again. Both sleep forever. The generation belongs to one round and only grows: A waits for "generation changed" – which stays true no matter what round 2 does.

</details>

<details><summary>Why is there no lost wake-up between incrementing <code>waiting</code> and sleeping?</summary>

Incrementing `waiting`, checking it and going to sleep happen under the same Mutex, and `Condition::wait` releases the Mutex and sleeps **in one step**. The last thread can only increment the generation and broadcast after it got the Mutex – i.e. after every earlier thread is really asleep (or has not arrived yet). No broadcast can fall into the gap.

</details>

<details><summary>Which thread gets the serial value, and why is that fine?</summary>

The **last** one to arrive – it opens the barrier (`waiting == count`). POSIX only demands that exactly one thread per round gets `PTHREAD_BARRIER_SERIAL_THREAD`, not which one; the last one is the natural choice because it is the only one that knows the round is complete.

</details>

<details><summary>Could you implement the barrier in user space with the syscalls you have?</summary>

Yes: a counter + generation in shared user memory, protected by your A1-15 lock, waiters yield in a loop until the generation changes (or use the A1-17 condition variables). It works, but waiting threads keep getting scheduled just to check – the kernel version sleeps on a Condition and costs nothing while waiting.

</details>
