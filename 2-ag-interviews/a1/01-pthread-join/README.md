# A1-01 · pthread_join

**Time box 60 min · ★★★ · builds on A0-08 (pthread_create) · your own P2 task**

```
Name:         pthread_join (+ pthread_exit keeps its value)
Nr:           1600 (sc_pthread_join)
Description:  pthread_join(thread, &value) waits until the thread has ended and stores
              the value it passed to pthread_exit (or returned from start_routine) in
              *value (if value is not NULL). A thread can be joined exactly once.
              A thread killed by a fault joins with value -1.
              The resources of an ended thread (its stack slot) can be reused once it
              has been joined - more than 64 create/join cycles must work.
Parameters:   pthread_t thread, void** value_ptr
Return Value: 0, or -1: unknown thread, already joined, or the caller itself
Notes:        Base: the A0-08 solution (task.sh start a1-01 sets it up).
              Thread ids: unique, never reused (slots are reused).
```

## Test

`task.sh start a1-01`, then `task.sh test` (runs `pthread_join_basic.sweb`,
`pthread_create_basic.sweb`, then `exit` for the leak check). Done when all 9 + 8 checks pass,
nothing hangs, and no `leaking` warning appears.

## Hints

<details><summary>1 – where does the exit value live?</summary>

The thread object is deleted by the CleanupThread some time after the thread ended – the
joiner may come much later. So the value has to be stored somewhere that outlives the
thread: in the process. The solution gives every stack slot a small record
(`state` FREE/RUNNING/FINISHED, `id`, `exit_value`) in `UserProcess`.
</details>

<details><summary>2 – who changes the state, when?</summary>

`pthread_exit` stores the value in the thread object. The thread's destructor (CleanupThread,
after the thread can never run again, after its stack is unmapped) copies it into the slot,
sets FINISHED and wakes the joiners. `pthread_join` sets FREE – only then the slot (and its
stack area) may be reused.
</details>

<details><summary>3 – how does join wait?</summary>

Mutex + Condition in the process: `while (slot is RUNNING) cond.wait();` – and broadcast in
`threadFinished`, because several threads may wait (for different threads, and the main
thread's `exit` waits for all).
</details>

<details><summary>4 – the trap: the slot changes while you sleep</summary>

While a joiner sleeps, a second joiner may join the same thread, free the slot, and a new
thread may get it. After every wake-up, check that the slot still has *your* id.
</details>

## Questions a tutor might ask

- Why can the exit value not stay in the thread object?
- Why `broadcast`, not `signal`?
- What happens if two threads join the same thread at the same time?
- Why is the slot freed in join and not when the thread ends?
- Joining yourself – why must that fail, and how do you detect it?
- In your team repo: where is this state, which lock protects it?
