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

`task start a1-01`, then `task test` (runs `pthread_join_basic.sweb`, `pthread_create_basic.sweb`,
then `exit` for the leak check).

## Your team's code: a study task

Your team already has `pthread_join` (your own P2 work) – `task solution a1-01` is `main` plus the
course tests, nothing else. **Run them against your code and explain every result.** On
`40140475` the first 8 checks pass, then the kernel **panics** on the 9th
("a killed thread can be joined"): see [the notes](../../solutions/a1/01-pthread-join/NOTES.md) –
that is a real bug in your code, and the notes say how to fix it.

Where to look:

| What | Your team's code |
|---|---|
| syscall | `Syscall::pthreadJoin` (`Syscall.cpp`): checks `value_ptr`, writes it after the join |
| the join | `UserProcess::joinThread(tid, &value)` under `thread_lock_` |
| per-thread record that outlives the thread | `UserThreadData` in `threads_`: `tid_`, `exit_value_`, `exited_`, `joined_` |
| the end of a thread | `UserProcess::exitThread`: frees the stack, stores the value, `exited_ = true`, `thread_exited_.broadcast()` |
| waiting | `while (!thread_data->exited_) thread_exited_.wait();` |
| process end | `exitProcess` kills every other thread (`kill()`), deletes their `UserThreadData` |

## Questions a tutor might ask

- Why can the exit value not stay in the thread object?
- Why `broadcast`, not `signal`?
- What happens if two threads join the same thread at the same time?
- Why is the slot freed in join and not when the thread ends?
- Joining yourself – why must that fail, and how do you detect it?
- Where is this state in your code, and which lock protects it?
- What happens to a thread that sleeps in `pthread_join` when another thread calls `exit`?
