# A1-23 pthread_switch (swap two thread ids) – solution notes

Patch: [`solution.patch`](solution.patch) – on your team's code (`f78faff3`). Type S.

## The code (simplest version)

```cpp
size_t UserProcess::switchThreadIds(size_t a, size_t b)
{
  ScopeLock lock(thread_lock_);              // ids, records and thread objects change together

  if (a == b)
    return -1;
  UserThreadData* data_a = findThreadDataByTid(a);
  UserThreadData* data_b = findThreadDataByTid(b);
  if (!data_a || !data_b)
    return -1;                               // unknown (or already joined and deleted)
  if (data_a->joined_ || data_b->joined_)
    return -1;                               // someone waits for that id right now

  data_a->tid_ = b;                          // the record follows the thread ...
  data_b->tid_ = a;
  if (data_a->thread_)                       // ... and so does the thread object, if it still runs
    data_a->thread_->setTID(b);
  if (data_b->thread_)
    data_b->thread_->setTID(a);
  return 0;
}
```

`Syscall::pthreadSwitch(a, b)` only calls it on `currentThread->getProcess()`; the libc wrapper
`pthread_switch(a, b)` in `pthread.c` (declared in `pthread.h`) calls `__syscall(sc_pthread_switch, a, b, …)`.

## Why each step

- **Two places hold the id:** the record (`UserThreadData::tid_`, searched by join/cancel/detach)
  and the thread object (`Thread::tid_`, returned by `pthread_self`). Both change.
- **One critical section under `thread_lock_`:** the same lock that exit, join and detach use for
  the records – nobody can delete a record or start a join between the lookup and the swap.
- **`thread_` may be `nullptr`:** an ended, unjoined thread only has its record left; its id is
  still swapped (a later join with the new id gets its value).
- **Refused while being joined (`joined_`):** the joiner already holds that record.
- **The flags stay with the thread:** `requested_cancel_`, `detached_`, `exited_` and the exit value
  are in the record, and the record moves with the id – so they belong to the work, not to the name.
- **The caller can be one of them:** its own `Thread::tid_` changes while it is in the syscall –
  harmless, it is only read later (`pthread_self`, cancellation check by tid).

## The test (9 checks)

Two threads wait for a flag, `pthread_switch(a, b)`, then each stores its `pthread_self()`: the one
that runs `workA` must now see `b`. `join(a)` returns `workB`'s result. Errors: `(a, a)`, an unknown
id. Then the main thread swaps itself with a new thread: `pthread_self()` changes, and
`join(old main id)` returns the other thread's result.
