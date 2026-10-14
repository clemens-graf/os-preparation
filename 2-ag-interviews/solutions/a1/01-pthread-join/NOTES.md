# A1-01 pthread_join – study notes (your team's code)

Patch: [`solution.patch`](solution.patch) is **empty** – your team's `main` already has
`pthread_join` (your own P2 work). `task solution a1-01` = `main` + the course tests.

## How your team's join works

```cpp
size_t UserProcess::joinThread(size_t tid, void** exit_value)
{
  ScopeLock lock(thread_lock_);
  if (tid == currentThread->getTID()) return -1;            // joining yourself
  UserThreadData* thread_data = findThreadDataByTid(tid);
  if (!thread_data) return -1;                              // unknown (or joined before: deleted)
  if (thread_data->joined_) return -1;                      // someone else joins it already
  thread_data->joined_ = true;
  while (!thread_data->exited_)
    thread_exited_.wait();                                  // broadcast in exitThread
  *exit_value = thread_data->exit_value_;
  removeThreadDataFromList(thread_data);
  delete thread_data;
  return 0;
}
```

- The record that outlives the thread is `UserThreadData` (in `threads_`): the thread object is
  deleted by the CleanupThread whenever it likes; the joiner may come much later.
- `exitThread` (under `thread_lock_`): frees the stack pages, stores `exit_value_`, sets
  `thread_ = nullptr` and `exited_ = true`, `thread_exited_.broadcast()` – broadcast, because
  several threads may wait for different threads.
- `joined_` is set *before* sleeping: a second joiner of the same thread fails at once instead
  of both waiting and both deleting the record.
- `Syscall::pthreadJoin` writes `*value_ptr` only after `joinThread` returned (lock released) –
  writing user memory may page-fault, and the page fault handler needs `thread_lock_`.

## What the course test shows: 8 pass, then a kernel panic

Check 9 creates a thread that dereferences NULL while `main` sleeps in `pthread_join`:

```
crash thread: page fault → Syscall::exit(9999) → UserProcess::exitProcess()
  → kill() on every other thread – also on main, which sleeps in thread_exited_.wait()
main is marked ToBeDestroyed, but it is still in the waiters list of thread_exited_
last thread gone → ~UserProcess → ~Condition: "still waiting on this lock" → assert → PANIC
```

`Thread::kill()` only sets the state. A thread that sleeps on a Condition (or waits for a Mutex)
is still in that lock's waiters list – and the lock dies with the process.

**Fix direction** (in `exitProcess`, while holding `thread_lock_`): wake everybody first, then kill:

```cpp
thread_exited_.broadcast();   // takes every sleeping joiner out of the waiters list
// ... then the existing loop: kill() every other thread
```

A broadcast thread is set to Running, then `kill()` sets ToBeDestroyed – it never runs again, but
it is no longer in a waiters list. (The cleaner design: like cancel – set a "process exits" flag,
wake everybody, and let every thread end itself at its next safe point.) Test it with the
course test: check 9 should then print `[PASS] a killed thread can be joined (value -1)` – or, with
your POSIX design (`exit` ends the whole process), at least no panic.

## Questions a tutor might ask

- Why can the exit value not stay in the thread object?
- Why `broadcast`, not `signal`?
- Two threads join the same thread at the same time – what happens in your code?
- What happens to a thread that sleeps in `pthread_join` when another thread calls `exit`?
  (Now you know: in your code, a panic – and why.)
