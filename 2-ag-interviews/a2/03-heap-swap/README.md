# A2-03 · swap onto the kernel heap

**Time box 25 min · ★★☆ · builds on A2-01**

```
Name:         swap-out onto kernel-heap
Description:  Instead of the swap device, a swapped-out page is saved on the kernel heap.
              The kernel heap is small and the kernel needs it itself: at most 64 pages may
              be swapped out this way; swapout fails when the limit is reached.
              The interface of A2-01 (swapout, swapinfo, page fault) stays the same.
```

## Test

`task.sh start a2-03` → `task.sh test`. Done when 13 checks pass, including "exactly 64 heap slots".

## Hints

<details><summary>1 – only one class changes</summary>

Everything above `SwapManager` (PTE marker, page fault, exit cleanup) stays. A slot becomes a
`char*` from `new char[PAGE_SIZE]`; `readIn` copies it back; `freeSlot` deletes it.
</details>

<details><summary>2 – running out of heap</summary>

`new` in SWEB asserts when the kernel heap is exhausted – refuse before that (a fixed limit).
</details>

## Questions a tutor might ask

- Why is the kernel heap a bad place for swapped pages in general?
- Which lock protects the buffer table? Is `new` under that lock OK?
