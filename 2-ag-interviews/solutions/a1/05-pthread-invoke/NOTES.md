# A1-05 pthread_invoke – solution notes

Patch: [`solution.patch`](solution.patch) – on your team's code.

## The code

```cpp
// Scheduler
void Scheduler::yieldTo(Thread* thread) { preferred_ = thread; yield(); }

// in schedule():
Thread* preferred = preferred_;
preferred_ = nullptr;
auto it = threads_.begin();
while (preferred && it != threads_.end() && !(*it == preferred && (*it)->schedulable()))
  ++it;
if (!preferred || it == threads_.end())
  for (it = threads_.begin(); it != threads_.end(); ++it)      // normal round robin
    if ((*it)->schedulable()) break;
if (it != threads_.end())
  currentThread = *it;
```

```cpp
// Syscall::pthreadInvoke
{ ScopeLock lock(process->getThreadLock()); target = process->findThreadData(id)->thread_; ... }
Scheduler::instance()->yieldTo(target);          // after the release: never yield holding a lock
```

## The pointer that may dangle

After the lock is released, the target may end and be deleted before `schedule()` runs.
`schedule()` therefore **compares** `preferred_` with the pointers in its own list and touches it
only if it is found there – every list member is alive (the CleanupThread removes threads from
the list before deleting them). Worst case (address reused by a new thread): that one runs first –
harmless.

## Result

Target first after invoke: 20/20; after a plain `sched_yield`: 4/20.

## Shortest working version (tested)

[`short.patch`](short.patch) – **34 lines** instead of 66, passes the same test. Get it with
`task short a1-05` (branch `short/…`, then `task test`).

What is shorter: `preferred_` is set directly instead of a `yieldTo()` method (the syscall is its own
function `Syscall::pthreadInvoke`). In `schedule()` your team's loop stays **untouched** – 5 lines go
in front of it:

```cpp
  auto it = threads_.begin();
  while (it != threads_.end() && *it != preferred_)     // A1-05: look for the invoked thread
    ++it;
  preferred_ = nullptr;
  if (it == threads_.end() || !(*it)->schedulable())
    it = threads_.begin();                              // not found / can't run: round robin as before
  for(; it != threads_.end(); ++it)                     // your team's loop, unchanged
  {
    if((*it)->schedulable())
    {
      currentThread = *it;
      break;
    }
  }
```

Found and runnable → the loop takes it in its first step. Otherwise `it` starts at `begin()` again:
plain round robin. `preferred_ = nullptr` right after the search: the wish counts once.

Use it to see the **core** of the task. In the interview, the full solution's extra checks are what
a tutor asks about ("what if the pointer is invalid / two threads at once?").
