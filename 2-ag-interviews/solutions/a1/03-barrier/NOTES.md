# A1-03 barriers – solution notes

Patches: [`on-top-of-a1-01.patch`](on-top-of-a1-01.patch), [`solution.patch`](solution.patch).

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
