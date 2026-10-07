# A1-03 · pthread barriers

**Time box 40 min · ★★★ · builds on A1-01 · man page: pthread_barrier_wait**

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

## Hints

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

## Questions a tutor might ask

- Why the generation counter? Draw the round where `waiting == 0` breaks.
- Why is there no lost wake-up between incrementing `waiting` and sleeping?
- Which thread gets the serial value, and why is that fine?
- Could you implement the barrier in user space with the syscalls you have?
