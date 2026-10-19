# A1-01 · pthread_join

**Time box 60 min · ★★★ · builds on A0-08 (pthread_create) · your own P2 task**

**Theory first:** [concepts.pdf](../../concepts.pdf) §15 a thread's life, §13 semaphores & condvars

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

`task start a1-01`, then `task test` (runs `pthread_join_basic.sweb`, `pthread_create_basic.sweb`,
then `exit` for the leak check).

## Your team's code: a study task

Your team already has `pthread_join` (your own P2 work) – `task solution a1-01` is `main` plus the
course tests, nothing else. **Run them against your code and explain every result.** On
`f78faff3` (19 Oct) the first 8 checks pass, then the kernel **panics** on the 9th
("a killed thread can be joined"): see [the notes](../../solutions/a1/01-pthread-join/NOTES.md) –
that is a real bug in your code, and the notes say how to fix it.

Where to look:

| What | Your team's code |
|---|---|
| syscall | `Syscall::pthreadJoin` (`Syscall.cpp`): checks `value_ptr`, writes it after the join |
| the join | `UserProcess::joinThread(tid, &value)` under `thread_lock_` |
| per-thread record that outlives the thread | `UserThreadData` in `threads_`: `tid_`, `exit_value_`, `exited_`, `joined_`, `detached_` |
| cancelled while joining | the wait loop checks the caller's own `requested_cancel_` → `JOIN_CANCELED` (since 18 Oct) |
| the end of a thread | `UserProcess::exitThread`: frees the stack, stores the value, `exited_ = true`, `thread_exited_.broadcast()` |
| waiting | `while (!thread_data->exited_) thread_exited_.wait();` |
| process end | `exitProcess` kills every other thread (`kill()`), deletes their `UserThreadData` |

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why can the exit value not stay in the thread object?</summary>

The thread object (`UserThread`) is deleted by the CleanupThread at some point after the thread ended – the joiner may come much later, or not at all yet. So the value is stored in a separate record that outlives the thread: `UserThreadData` (`exit_value_`, `exited_`) in the process's `threads_`, removed only by the join.

</details>

<details><summary>Why <code>broadcast</code>, not <code>signal</code>?</summary>

All joiners of all threads of the process sleep on the **same** Condition `thread_exited_`. `signal` wakes one – maybe one waiting for a *different* thread, which checks, sees its thread still runs, and sleeps again; the right one never wakes. `broadcast` wakes all; each re-checks its own `exited_` in the `while` loop.

</details>

<details><summary>What happens if two threads join the same thread at the same time?</summary>

`joined_` is set under `thread_lock_` before the first joiner sleeps. The second joiner finds `joined_ == true` and gets -1 at once. Without it both would wait, both get the value, and both would `delete` the record – a double free.

</details>

<details><summary>Why is the slot freed in join and not when the thread ends?</summary>

In your team's code the **stack pages** are freed already when the thread ends (`exitThread` → `freeUserThreadStack`): nobody runs on them any more. The **record** (`UserThreadData`) is freed in join, because it carries the exit value that the joiner still has to collect – freeing it at exit would lose the value.

</details>

<details><summary>Joining yourself – why must that fail, and how do you detect it?</summary>

Joining yourself would wait for your own end, which can't happen while you wait: a deadlock. Detect it by comparing ids: `tid == currentThread->getTID()` → -1 (POSIX: `EDEADLK`).

</details>

<details><summary>Where is this state in your code, and which lock protects it?</summary>

`UserThreadData` in `UserProcess::threads_` (`tid_`, `exit_value_`, `exited_`, `joined_`, `detached_`); the Condition `thread_exited_`. All protected by the process's `thread_lock_` (`getThreadLock()`) – the same Mutex that `thread_exited_` uses.

</details>

<details><summary>What happens to a thread that sleeps in <code>pthread_join</code> when another thread calls <code>exit</code>?</summary>

In your team's code: **a kernel panic.** `exitProcess` calls `kill()` on the sleeping joiner – that only sets its state; it is still in the waiters list of `thread_exited_`. When the last thread is gone, `~UserProcess` destroys the Condition with a waiter still listed → "still waiting on this lock" assertion. Fix: `thread_exited_.broadcast()` in `exitProcess` before the kill loop (or end every thread at its next safe point, like cancel).

</details>
