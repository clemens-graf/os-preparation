# A2-01 · swap a page out and back in

**Time box 60 min · ★★★ · type S + E (page fault) ([TRIAGE](../../TRIAGE.md)) · read first: ex08 (disk), ex04 (PTE bits)**

```
Name:         swapout, swapinfo
Nr:           1720, 1721
Description:  swapout(address) writes the page containing address to a free slot of the
              swap partition idea2, frees its physical page and marks the page-table
              entry as "swapped out, slot N". The next access to the page (from user or
              kernel mode) brings it back: the page fault handler reads the slot into a
              new physical page, maps it again and frees the slot.
              swapinfo() returns the number of slots in use (all processes).
              When a process exits, the slots of its swapped-out pages are freed.
Parameters:   void* address
Return Value: 0, or -1 (not mapped / already swapped out / device full)
```

## Test

`task.sh start a2-01` → `task.sh test`. Done when 10 checks pass: 8 pages out and back, slots
counted and freed, the program's own stack and code page swapped out, slots freed at exit, no leak.

## Your team's code

`SwapManager` (new, `common/{include,source}/mm/`, `init()` in `startup()` after `doDeviceDetection`). `swapOut(vpn)` / `swapIn(vpn)` in `UserProcess` under `thread_lock_` (your page-table lock). The swap-in case goes **first** in the page fault handler's valid branch (before the stack/guard checks and `loadPage`), then `return`. `~UserProcess` frees the slots before `delete arch_memory_`.

## Hints

<details><summary>1 – where is 'swapped, slot N' stored?</summary>

In the page-table entry itself: `present = 0` (the CPU then ignores every other bit), one of the
free `ignored` bits as marker, the slot number in `page_ppn`. No extra table needed.
</details>

<details><summary>2 – the order in swapOut</summary>

1. `present = 0` + TLB flush – **first**, so no thread can write the page while it is copied;
2. write it to disk (keep holding the page-table lock: a thread touching the page faults and waits);
3. slot into the entry, marker on, free the physical page.
</details>

<details><summary>3 – the trap in unmapPage</summary>

`ArchMemory::unmapPage` frees a page table once no entry in it is *present*. A swapped-out page
is not present – its entry (with the slot!) would be thrown away with the table. Change the
"table empty?" check to "all entries zero?".
</details>

<details><summary>4 – the swap device</summary>

A `SwapManager` created in `startup()` after `BDManager` detected the devices: device = `idea2`,
a `Bitmap` of slots (slot s at byte offset s × 4096), a Mutex. Read/write through the page's ident
address – it does not have to be mapped anywhere.
</details>

<details><summary>5 – process exit</summary>

`~UserProcess` (before `arch_memory_` is destroyed): walk the tables, free the slot of every swapped
entry. Otherwise the leak shows up as `swapinfo() != 0` after the process is gone.
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why present = 0 BEFORE writing to disk? What breaks otherwise?</summary>

Writing to disk takes time and the process's other threads keep running. If the page were still present, a thread could write to it **during** the disk write: the copy on disk would be torn (half old, half new), and after swap-in that write is lost. `present = 0` + `flushTlb()` first makes every access fault – and the fault waits on `thread_lock_` until the swap-out is finished.

</details>

<details><summary>Two threads fault on the same swapped page. Walk through both.</summary>

Thread A faults, takes `thread_lock_`, sees "swapped", allocates a frame, reads the slot (sleeps during the disk read – still holding the lock), maps the page, `present = 1` last, releases. Thread B faulted meanwhile and waited for the lock; it then finds the page **present** → `swapIn` returns `true` ("someone was faster") – nothing to do, its instruction is repeated and works.

</details>

<details><summary>Which locks are held during the disk write? Why is that allowed?</summary>

The process's `thread_lock_` (a Mutex) and inside the device request the `SwapManager`'s lock. Allowed because both are Mutexes: the disk request **sleeps** (yields until the IDE interrupt), it doesn't spin. Holding `thread_lock_` blocks this process's page faults meanwhile – exactly what keeps them away from the half-done page. A SpinLock here would be wrong.

</details>

<details><summary>Why does a kernel-mode fault (syscall writes into a swapped page) also work?</summary>

The page fault handler runs with interrupts on in the faulting thread's context – no matter whether the fault came from user code or from the kernel touching a user buffer for a syscall. Both take the same path: `swapIn` reads the page back, the kernel instruction is repeated.

</details>

<details><summary>What does your team's A2 design do instead of a syscall (a swap thread, a policy)?</summary>

In the real A2 no program asks for swapping: when `allocPPN` finds no free frame (or a kernel swap thread sees memory running low), the kernel itself picks a victim page with a **replacement policy** (FIFO, clock/second chance using the accessed bit, …) and swaps it out. The syscall in this task is only a way to trigger it deterministically for the test.

</details>
