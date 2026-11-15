# A1-12 sbrk – solution notes

Patch: [`solution.patch`](solution.patch). Same mechanism as A0-03 (heaplimit), different API.

```cpp
size_t Syscall::sbrk(ssize_t increment)
{
  ScopeLock lock(process->getThreadLock());
  size_t old_brk = process->brk_;
  size_t new_brk = old_brk + increment;
  if (new_brk < BRK_START || new_brk > BRK_START + BRK_MAX)   // also wrap-around
    return (size_t) -1;
  if (increment < 0)
    /* unmap pages [roundup(new), roundup(old)), flush TLB */;
  process->brk_ = new_brk;
  return old_brk;
}
```

Page fault: addresses in `[BRK_START, BRK_START + BRK_MAX)` → `mapBrkPage(vpn)`: check `vpn * 4096 <
brk_` and map **in one critical section**, else exit 9997 (after releasing the lock).

libc: the existing stub `void* sbrk(intptr_t)` in `unistd.c` – the kernel's `(size_t) -1` is
exactly `(void*) -1`.

## A tiny malloc on top

```c
void* malloc(size_t n) { n = (n + 15) & ~15; void* p = sbrk(n); return p == (void*) -1 ? 0 : p; }
```

(no free) – a real one keeps a free list and only grows the break when needed.

## Shortest working version (tested)

[`short.patch`](short.patch) – **43 lines** instead of 67, passes the same test. Get it with
`task short a1-12` (branch `short/…`, then `task test`).

What is shorter: the same logic without comments; the shrink loop runs for growing too (it is simply empty then).

Use it to see the **core** of the task. In the interview, the full solution's extra checks are what
a tutor asks about ("what if the pointer is invalid / two threads at once?").
