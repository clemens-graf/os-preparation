# A1-02 pthread_multiple + pthread_join_any – solution notes

Patches: [`on-top-of-a1-01.patch`](on-top-of-a1-01.patch) (read this), [`solution.patch`](solution.patch).

## pthread_multiple: all or nothing

```cpp
// 1. check + copy in (args), touch the ids array - before anything exists
// 2. create every thread object - NOT scheduled, so none can run
for (i...) {
  created[i] = process->createThread(entry, start_routine, arg_copy[i]);
  if (!created[i]) {
    for (j < i) process->discardThread(created[j]);   // roll back
    return -1;
  }
}
// 3. ids out, 4. only now: addNewThread for all of them
```

`discardThread`: `delete` the never-scheduled thread (its destructor unmaps the stack and marks
the slot FINISHED), then mark the slot FREE – nobody knows its id. Deleting a thread yourself
is only OK because it never ran: it is in no list, holds no lock, has no state anywhere else.

## pthread_join_any

All under `threads_lock_`, in a loop: is any of the ids FINISHED → join it (FREE, value, index).
Otherwise: is any of them still RUNNING → `wait()` (woken by `threadFinished`'s broadcast);
none → return -1 (waiting would never end).

## Locking / pitfalls

- Copy the ids array into the kernel first (TOCTOU: another thread may change it while you sleep).
- Writing `*index` / `*value` to user memory only after `joinAny` returned (lock released).
- Adding each thread to the scheduler right after creating it is the bug the test catches:
  the first threads run although creation fails later.
- Test lesson: busy loops finish within one 55 ms time slice in QEMU; the test uses yields to get
  a deterministic finishing order.
