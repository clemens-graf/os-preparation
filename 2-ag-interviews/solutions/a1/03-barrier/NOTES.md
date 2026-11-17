# A1-03 barriers – solution notes

Patch: [`solution.patch`](solution.patch) – on your team's code.

## The data

```cpp
struct Barrier { bool used; size_t count; size_t waiting; size_t generation; };
Mutex barriers_lock_;            // protects barriers_
Condition barrier_opened_;       // broadcast whenever ANY barrier opens
Barrier barriers_[MAX_BARRIERS];
```

The user's `pthread_barrier_t` holds the barrier's id (index + 1, 0 = invalid).

## barrierWait

```cpp
ScopeLock lock(barriers_lock_);
Barrier& b = barriers_[id - 1];
size_t my_generation = b.generation;
if (++b.waiting == b.count) {        // the last one opens it
  b.waiting = 0;
  ++b.generation;
  barrier_opened_.broadcast();
  return 1;                          // PTHREAD_BARRIER_SERIAL_THREAD
}
while (b.generation == my_generation)
  barrier_opened_.wait();
return 0;
```

## Why the generation

With `while (b.waiting != 0) wait();`: the last thread sets `waiting = 0` and broadcasts. Before
the sleepers run, a fast thread loops around and arrives at the NEXT round: `waiting = 1`. The
sleepers wake, see 1, sleep again – forever. The generation number belongs to one round and only
goes up – "has my round opened?" is answered correctly no matter what happens afterwards.

## Locking

One Mutex; the increment, the check and the opening are one critical section, so there is no
lost wake-up: a thread either increments before the last one (and then waits for the generation
the last one changes under the same lock) or it is the last one. Broadcast, because all of the
round must wake.

## Shortest working version (tested)

[`short.patch`](short.patch) – **91 lines** instead of 142, passes the same test. Get it with
`task short a1-03` (branch `short/…`, then `task test`).

What is simpler:
- `thread_lock_` + one Condition instead of a separate barrier lock.
- **libc keeps the id**, the kernel only sees numbers: `pthread_barrier_init` gets the id back from
  the syscall and writes it into `*barrier` itself; `wait`/`destroy` pass `*barrier` by value. So the
  kernel never touches user memory – no pointer checks, and the three `case`s are one line each:

```cpp
// libc
size_t id = __syscall(sc_barrier_init, count, 0x00, 0x00, 0x00, 0x00);
if (id == (size_t) -1)
  return -1;
*barrier = id;
...
return __syscall(sc_barrier_wait, *barrier, 0x00, 0x00, 0x00, 0x00);

// Syscall.cpp
case sc_barrier_wait:
  return_value = process->barrierWait(arg1);       // arg1 = id
  break;
```

- The kernel part is already the minimum: a table of `{count, waiting, round}`, the last thread
  resets `waiting`, increments `round` and broadcasts; the others wait `while (round == my_round)`.
  Without `round`, a fast thread entering the next round could make the slow ones sleep forever.

A tutor may ask: "what if the program passes a bad id?" – the kernel checks the id (0, > 16, free
slot) and returns -1. A bad *pointer* only crashes the program itself, in libc – like any bad pointer
it dereferences.
