# A1-05 pthread_invoke – solution notes

Patches: [`on-top-of-a1-01.patch`](on-top-of-a1-01.patch), [`solution.patch`](solution.patch).

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
{ ScopeLock lock(process->getThreadsLock()); target = process->findThread(id); ... }
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
