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

## Your team's code

Builds on A2-01 and only changes `SwapManager` – identical in your team's code.

## Hints

<details><summary>1 – only one class changes</summary>

Everything above `SwapManager` (PTE marker, page fault, exit cleanup) stays. A slot becomes a
`char*` from `new char[PAGE_SIZE]`; `readIn` copies it back; `freeSlot` deletes it.
</details>

<details><summary>2 – running out of heap</summary>

`new` in SWEB asserts when the kernel heap is exhausted – refuse before that (a fixed limit).
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why is the kernel heap a bad place for swapped pages in general?</summary>

Swapping exists because **RAM is full** – and the kernel heap *is* RAM. Moving a page from a frame into the heap frees nothing overall (and costs a copy). Plus the kernel heap is small and the kernel needs it itself: if it runs out, `new` asserts and the whole kernel stops. Useful only as an exercise: it separates the bookkeeping from the device.

</details>

<details><summary>Which lock protects the buffer table? Is <code>new</code> under that lock OK?</summary>

`SwapManager::lock_` (a Mutex) protects `buffers_[]`. `new` under it is fine: the kernel heap has its own lock inside, and the order is always SwapManager → heap, never the reverse – no cycle. And a Mutex may be held while `new` takes time; with interrupts off (in a SpinLock or the scheduler) it would not be allowed.

</details>
