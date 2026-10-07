# A1-01 pthread_join – solution notes

Patches: [`solution.patch`](solution.patch) (complete, against upstream: includes A0-08) and
[`on-top-of-a0-08.patch`](on-top-of-a0-08.patch) (only the join part – read this one).

## Files touched (on top of A0-08)

| File | What |
|---|---|
| `UserProcess.h/.cpp` | `ThreadSlot { state, id, exit_value }` per stack slot, `next_thread_id_`, `threads_changed_` (Condition, broadcast); `createThread` picks a FREE slot; `threadFinished(slot, value)`; `joinThread(id, &value)` |
| `UserThread.h/.cpp` | `thread_id_` (monotonic) separate from `stack_slot_` (reused); `exit_value_` (default -1) |
| `Syscall.cpp` | `pthreadExit` stores the value in the thread; `pthreadJoin` |
| `pthread.c` | `pthread_join` wrapper |

## The life of a slot

```
FREE --createThread--> RUNNING --~UserThread / threadFinished--> FINISHED --joinThread--> FREE
                         (thread exists)        (stack unmapped, value stored)   (value handed out)
```

- The value moves: `pthread_exit` → `UserThread::exit_value_` → (destructor) → `slots_[s].exit_value`
  → joiner. The thread object cannot hold it: the CleanupThread deletes it whenever it likes.
- The slot is only reused after the join, so the stack area is free and the id cannot be confused.

## joinThread

```cpp
size_t UserProcess::joinThread(size_t id, size_t* exit_value)
{
  ScopeLock lock(threads_lock_);
  /* find slot with this id (not FREE) -> none: return -1 */
  if (currentThread != this && ((UserThread*) currentThread)->getThreadId() == id)
    return (size_t) -1;                              // self-join would wait forever
  while (slots_[slot].state == SLOT_RUNNING && slots_[slot].id == id)
    threads_changed_.wait();
  if (slots_[slot].state != SLOT_FINISHED || slots_[slot].id != id)
    return (size_t) -1;                              // another joiner was faster
  *exit_value = slots_[slot].exit_value;
  slots_[slot].state = SLOT_FREE;
  return 0;
}
```

## Locking

- One Mutex (`threads_lock_`) for all slots + counters; one Condition, **broadcast** in
  `threadFinished`: joiners of *different* threads and the main thread in `exit()` all wait on it,
  each re-checks its own condition.
- The re-check after `wait()` includes the **id**: while a joiner sleeps, another joiner can join
  the same thread and a new thread can take the slot.
- `Syscall::pthreadJoin` writes the user's `value_ptr` **after** `joinThread` returned (lock
  released): that write may page-fault.
- `threadFinished` is the last thing `~UserThread` does: after it, the main thread may run to
  completion and delete the loader.

## Pitfalls

- Storing the value in the thread and reading it in join → use after free (cleanup already ran).
- Freeing the slot when the thread ends (not at join) → join after the end finds nothing.
- `signal` instead of `broadcast` → the wrong waiter wakes up, the right one sleeps forever.
- Busy waiting (`while (alive) yield();`) – works, gets a remark.

## In the team repo

Same questions: where is the per-thread "finished + value" stored, who frees the thread object
and when, which lock. That is literally your P2 design.
