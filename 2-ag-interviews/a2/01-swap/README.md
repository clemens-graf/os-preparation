# A2-01 · swap a page out and back in

**Time box 60 min · ★★★ · read first: ex08 (disk), ex04 (PTE bits)**

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

`~Loader` (before `arch_memory_` is destroyed): walk the tables, free the slot of every swapped
entry. Otherwise the leak shows up as `swapinfo() != 0` after the process is gone.
</details>

## Questions a tutor might ask

- Why present = 0 BEFORE writing to disk? What breaks otherwise?
- Two threads fault on the same swapped page. Walk through both.
- Which locks are held during the disk write? Why is that allowed?
- Why does a kernel-mode fault (syscall writes into a swapped page) also work?
- What does your team's A2 design do instead of a syscall (a swap thread, a policy)?
