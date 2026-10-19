# A2-13 · anonymous mmap / munmap

**Time box 45 min · ★★★ · type S + E (syscall + page fault handler)**

```
Name:         mmap, munmap (POSIX, sys/mman.h - the stubs exist)
Nr:           1800 (sc_mmap), 1801 (sc_munmap)
Description:  mmap(NULL, length, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) reserves length
              bytes (rounded up to pages) of fresh, zeroed memory and returns its address.
              Nothing is mapped by the syscall: a page is mapped on its first access (page
              fault handler). Without PROT_WRITE the pages are read-only.
              munmap(address, length) removes a whole mapping (address = what mmap returned,
              the same length); afterwards any access to it kills the process.
              Only anonymous mappings: file mappings (fd != -1) fail.
Return Value: mmap: the address, or (void*) -1; munmap: 0 or -1
```

## Test

`task start a2-13` → `task test` (`mmap_basic.sweb`, then `exit`). Done when 11 checks pass and
the last access (to unmapped memory) kills the process.

## Your team's code

The stubs are in `userspace/libc/src/mman.c` (the `PROT_*` / `MAP_*` values in `sys/mman.h`).
`mmap` has 6 parameters, `__syscall` carries 5: drop `start` (the kernel chooses) and check
`offset == 0` in the libc. Per-process state (the list of mappings) → `UserProcess`, under
`thread_lock_`; the page-fault case goes **first** in the handler's valid branch (it takes the
lock itself) – the same pattern as A0-03 / A1-12.

## Hints

<details><summary>1 – what does the kernel need to remember?</summary>

Per mapping: first vpn, number of pages, writable. A small array in `UserProcess`, and a
"next free vpn" that only grows (from e.g. `0x700000000000`, far below the stacks).
</details>

<details><summary>2 – the page fault</summary>

`mapMmapPage(vpn)`: `allocPPN` before the lock; under `thread_lock_`: is vpn inside a mapping?
then `mapPage` (and clear `writeable` for read-only ones – same critical section); release;
`freePPN` if nothing was mapped. Not inside → return false, the handler goes on (→ 666).
</details>

<details><summary>3 – munmap</summary>

Under the lock: find the mapping, `unmapPage` every page that is present (that frees the
frames), `flushTlb()`, mark the entry free.
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why map lazily? What does <code>free</code> memory cost before it is touched?</summary>

Lazy is cheaper and faster: `mmap` of 1 GiB only reserves address space – a few lines in a table. Memory that is never touched costs **nothing** (no frame, no page table); each page costs a frame only at its first access. Linux does the same (overcommit).

</details>

<details><summary>munmap while another thread faults on the same page – walk through it.</summary>

`munmap` takes `thread_lock_`, removes the region and unmaps its pages (frees them), flushes. The faulting thread's `mapMmapPage` takes the same lock: **before** the munmap → it maps the page, then munmap unmaps it again; **after** → it finds no region → "not inside" → the process is killed (access to unmapped memory). Never a page mapped without a region, because check + map is one critical section.

</details>

<details><summary>What would a *file* mapping need? (the page fault handler reads the file – like <code>loadPage</code>)</summary>

The region stores the file (an open file + offset); the page fault handler reads the page's 4 KiB from the file into a fresh frame instead of zeroing it – exactly what `loadPage` does for the program binary. For writable shared file mappings also: write dirty pages back (msync, munmap, exit).

</details>

<details><summary>What would <code>MAP_SHARED</code> between two processes need? (A1-10: one frame, two page tables)</summary>

Both processes must map **the same frame** for the same region: the first fault allocates it and stores it (with a reference count) in a shared object; the other process's fault maps that frame instead of a new one; free only when the last mapping goes – A1-10's shared pages, per page.

</details>
