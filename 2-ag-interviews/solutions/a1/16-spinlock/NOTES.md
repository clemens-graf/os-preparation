# A1-16 spinlock + mutex in user space – solution notes

Patch: [`solution.patch`](solution.patch) – `userspace/libc/src/pthread.c` + the missing
declarations in `userspace/libc/include/pthread.h`. Type U.

```c
int pthread_spin_lock(pthread_spinlock_t* lock)
{
  if (!lock) return -1;
  while (__sync_lock_test_and_set(lock, 1))   // writes 1, returns the OLD value - one step
    sched_yield();                            // held: let the holder run, then try again
  return 0;
}
int pthread_spin_trylock(pthread_spinlock_t* lock) { ... return __sync_lock_test_and_set(lock, 1) ? -1 : 0; }
int pthread_spin_unlock(pthread_spinlock_t* lock)  { ... __sync_lock_release(lock); return 0; }   // = 0

int pthread_mutex_lock(pthread_mutex_t* mutex) { return pthread_spin_lock(mutex); }   // same thing
```

## Why the one atomic instruction

```c
while (*lock) sched_yield();   // A: sees 0 ...
                               //    <- timer interrupt: B also sees 0 and takes the lock
*lock = 1;                     // A: ... and takes it too. Both are inside.
```

`__sync_lock_test_and_set` compiles to `xchg`: read the old value and write 1 in one indivisible
step – nobody can come between. **The test cannot prove this:** the naive version above passes
it too (the window is a few instructions; the timer rarely hits exactly there). Such bugs survive
testing – the tutor asks *why* yours is correct.

- `atexit` (A1-14) gets away with a plain counter only because registering from two threads at
  the same moment is unusual – a lock is used by many threads at the same moment by definition.
- `sched_yield` in the loop: on one CPU, spinning without yielding wastes the whole time slice –
  the holder cannot run and unlock in the meantime.
- A real mutex sleeps instead (a syscall + a wait queue in the kernel, woken by unlock).
