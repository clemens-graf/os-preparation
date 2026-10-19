# A1-17 condition variables in user space – solution notes

Patches: [`on-top-of-a1-15.patch`](on-top-of-a1-15.patch) (read this – only `pthread.c`),
[`solution.patch`](solution.patch) (with the A1-15 mutex). Type U.

```c
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex)
{
  if (!cond || !mutex) return -1;
  volatile pthread_cond_t* counter = cond;                        // volatile: read from memory every time
  pthread_cond_t generation = *counter;                           // 1. while holding the mutex
  pthread_mutex_unlock(mutex);                                    // 2. let the signaller in
  while (*counter == generation)                                  // 3. wait for a new generation
    sched_yield();
  pthread_mutex_lock(mutex);                                      // 4. return holding it again
  return 0;
}

int pthread_cond_signal(pthread_cond_t* cond)    { if (!cond) return -1; __sync_fetch_and_add(cond, 1); return 0; }
int pthread_cond_broadcast(pthread_cond_t* cond) { return pthread_cond_signal(cond); }
```

- **Lost wake-up:** step 1 before step 2. If wait read the counter *after* unlocking, a signal
  could happen in between, and the waiter would wait for a second one that never comes.
- `volatile`: the loop must read memory every time, the other thread changes it.
- `signal` wakes everyone (every waiter sees the new generation) – allowed, because waiters
  re-check their condition in a loop (`while (queue == 0) wait`). A real implementation wakes
  exactly one sleeper from a wait queue.
- The atomic increment: `cond++` is read-modify-write; two signallers could both write the same
  new value – and a waiter that saw that value would not notice the second signal.
- Kernel analogue: SWEB's `Condition` (`wait` releases the Mutex and sleeps; `signal` wakes one
  thread from the waiters list) – exactly your team's `thread_exited_` in `joinThread`.
