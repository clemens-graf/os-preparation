# A1-12 · sbrk

**Time box 30 min · ★★☆**

**Theory first:** [concepts.pdf](../../concepts.pdf) §3 sbrk & demand paging, §4 page faults

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         sbrk (POSIX, unistd.h - the stub exists)
Nr:           1700
Description:  Every process has a program break, starting at 0x600000000000.
              sbrk(increment) moves it by increment bytes (may be negative) and returns
              the OLD break. Memory below the break is usable; it is mapped (zeroed) on
              first access by the page fault handler. Shrinking unmaps every page that is
              now completely above the break. An access above the break kills the process.
Return Value: old break, or (void*) -1 if the new break would be below the start or more
              than 4096 pages above it
```

## Test

`task.sh start a1-12` → `task.sh test`. Done when 7 checks pass and the last access above the
shrunk break kills the process.

## Level 2 – hints

<details><summary>1 – compare with A0-03</summary>

Same mechanism (page fault maps on demand, check + map in one critical section), but byte-granular and returning the old break.
</details>

<details><summary>2 – shrinking</summary>

Unmap the pages from `roundup(new_break)` to `roundup(old_break)`, then flush the TLB. A page that still holds bytes below the break stays.
</details>

<details><summary>3 – overflow</summary>

`old + increment` can wrap around – check the result against both ends of the region.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Same three pieces as A0-03: a per-process break `brk_` (starts at the heap start) in `UserProcess`, a syscall that moves it, and the page fault handler that maps on demand.
2. Syscall `sbrk(increment)` (1700, the libc stub in `unistd.c` already exists): under `thread_lock_` remember the old break, compute the new one.
3. Refuse a new break below the start or above the maximum (also for a negative increment that wraps around): return -1.
4. Shrinking: unmap every page that is now completely above the new break, flush the TLB.
5. Store the new break and return the **old** one.
6. Page fault: an address in the heap region → under `thread_lock_` check "below the break?" and map a zeroed frame in the **same** critical section; above → kill (after the lock).
7. `task test`.

</details>

## Your team's code

Like A0-03: `brk_ = BRK_START`, `mapBrkPage(vpn)` and `sbrk(increment)` in `UserProcess` (under `thread_lock_`). The heap case goes **first** in the page fault handler's valid branch, before it takes `thread_lock_` (`mapBrkPage` takes it itself), then `return`. The libc `sbrk` stub is in `userspace/libc/src/unistd.c`.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why return the old break?</summary>

The old break is the **start of the new memory**: `p = sbrk(4096)` gives you exactly the block you just got. `sbrk(0)` returns the current end without changing it. (A negative increment returns the old end too – the caller then knows what it gave back.)

</details>

<details><summary>How would malloc use sbrk? Why does free usually not shrink?</summary>

`malloc` asks for a big chunk with `sbrk` when its pool is empty and cuts blocks out of it (A1-14). `free` usually only marks a block free: the break can only shrink from the **top**, and a free block in the middle (with used blocks above it) can't be returned. Only when the topmost blocks are free could `malloc` shrink with `sbrk(-n)`.

</details>

<details><summary>Two threads call sbrk at the same time – what does your lock guarantee?</summary>

Reading the old break, checking the new one and storing it is one critical section under `thread_lock_` (the same lock the page fault handler uses for mapping heap pages). Two concurrent `sbrk(+n)` therefore get **disjoint** ranges, and a shrink can't happen between a fault's "below the break?" check and its `mapPage`.

</details>
