# A1-12 · sbrk

**Time box 30 min · ★★☆**

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

## Hints

<details><summary>1 – compare with A0-03</summary>

Same mechanism (page fault maps on demand, check + map in one critical section), but byte-granular and returning the old break.
</details>

<details><summary>2 – shrinking</summary>

Unmap the pages from `roundup(new_break)` to `roundup(old_break)`, then flush the TLB. A page that still holds bytes below the break stays.
</details>

<details><summary>3 – overflow</summary>

`old + increment` can wrap around – check the result against both ends of the region.
</details>

## Questions a tutor might ask

- Why return the old break?
- How would malloc use sbrk? Why does free usually not shrink?
- Two threads call sbrk at the same time – what does your lock guarantee?
