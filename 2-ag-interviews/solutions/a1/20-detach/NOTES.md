# A1-20 pthread_detach – solution notes

**Your team's `main` has this now (your own `feature/pthread-detach`)** – the solution patch is
empty, `task test` checks your team's code, which is the same as the code below (syscall 1504).

Patch: [`solution.patch`](solution.patch). Type S, on top of your team's join.

```cpp
size_t UserProcess::detachThread(size_t tid)
{
  ScopeLock lock(thread_lock_);
  UserThreadData* thread_data = findThreadDataByTid(tid);
  if (!thread_data || thread_data->detached_ || thread_data->joined_) return -1;
  if (thread_data->exited_)                  // ended already: only the record is left
  {
    removeThreadDataFromList(thread_data);
    delete thread_data;
    return 0;
  }
  thread_data->detached_ = true;             // exitThread frees it
  return 0;
}

// exitThread, under thread_lock_, after exited_ = true and the broadcast:
if (thread_data->detached_) { removeThreadDataFromList(thread_data); delete thread_data; }
// joinThread: if (thread_data->detached_) return -1;
```

- Everything under `thread_lock_` – the same lock `exitThread` holds when it sets `exited_`.
  So a thread is either "ended" (detach frees the record) or "detached" (exit frees it) – never
  ended-but-forgotten.
- Without detach, a thread nobody joins keeps its `UserThreadData` until the process ends
  (`exitProcess` deletes all of them) – a leak that grows with every such thread.
- The broadcast in `exitThread` stays before the delete: a joiner of a *detached* thread does not
  exist (join refuses), so nobody reads the record after it is gone.
