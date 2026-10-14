# A1-17 semaphores in user space – solution notes

Patches: [`on-top-of-a1-16.patch`](on-top-of-a1-16.patch) (read this – only `semaphore.c`),
[`solution.patch`](solution.patch) (with the A1-16 spinlock). Type U.

```c
static pthread_spinlock_t sem_lock = 0;      // ONE lock for all semaphores: sem_t is only a counter

int sem_trywait(sem_t* sem)
{
  if (!sem) return -1;
  pthread_spin_lock(&sem_lock);
  if (*sem > 0) { (*sem)--; pthread_spin_unlock(&sem_lock); return 0; }   // check + change: one lock
  pthread_spin_unlock(&sem_lock);
  return -1;
}

int sem_wait(sem_t* sem)
{
  if (!sem) return -1;
  while (sem_trywait(sem) != 0)
    sched_yield();                           // never wait while holding sem_lock
  return 0;
}

int sem_post(sem_t* sem) { ... lock; (*sem)++; unlock; return 0; }
```

- Check and decrement under the **same** lock – otherwise two waiters see `1` and both decrement.
- Waiting = yielding in a loop *without* the lock; holding it while waiting would block `sem_post`
  forever.
- A kernel semaphore would sleep on a wait queue and `sem_post` would wake one waiter
  (in SWEB's kernel: a Mutex + a `Condition`).
- One global lock for every semaphore is correct but serialises unrelated semaphores – fine here.
