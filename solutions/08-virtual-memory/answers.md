# Module 08 — Answers

**Q1.** Nine bits per level, twelve for the offset:
PML4 `0xfe` (254), PDPT `0x48` (72), PD `0x1a2` (418), PT `0x167` (359),
offset `0xabc`.
(a) Three frames: a PDPT, a PD and a PT (the PML4 already exists). That
is 12 KiB of tables for a 4 KiB page.
(b) 1 GiB = 262144 pages = 512 page tables of 512 entries each, plus 1 PD
and 1 PDPT: **514 frames, about 2 MiB** (0.2 % of the mapped memory). With
2 MiB huge pages, the PD entries would map the memory directly and no PTs
would be needed.

**Q2.** A flat table has 2³⁶ entries × 8 B = **512 GiB per process**,
almost all of it describing unmapped space. The tree only has tables for
the parts that are used. The price is up to **four extra memory reads per
access** (one per level) whenever the translation is not cached. The
**TLB** hides it, since typical programs hit it well over 99 % of the
time. Paging-structure caches (for the upper levels) and huge pages help
too.

**Q3.** The MMU combines the rights of all levels with AND. The effective
right is the most restrictive level, so permissive upper levels let the
leaf entry decide alone. With `user_access = 0` in a PML4 entry, every
page under that entry (512 GiB) becomes kernel-only. User accesses fault
with a *protection* error even though the leaf says U. SWEB uses exactly
that for the upper half of the PML4 (the kernel), copied into every
process in the `ArchMemory` constructor.

**Q4.** (a) The TLB caches *present* translations only. After an unmap,
a cached entry would keep the old frame reachable, even after that frame
has been freed and reused by another process. A page that was not present
cannot be in the TLB, so mapping it needs no invalidation. (b) The CPU
that changes the mapping interrupts all other CPUs that may use the
address space (inter-processor interrupt). Each one invalidates the entry
and acknowledges, and the initiator waits for everyone. That is expensive
because it means interrupts, synchronisation and waiting, on every unmap
and permission change; kernels batch the invalidations. SWEB runs on one
CPU, so `invlpg` or a CR3 reload suffices. (c) A write through the stale
*writable* entry does not fault. It lands in the frame that is now shared
with the child, and the child sees the parent's write: memory corruption
across processes. A stale entry is only harmless when it is *less*
permissive than the table, because the resulting fault makes the CPU walk
the table again.

**Q5.** The frame is shared. If the parent could still write, its writes
would appear in the child. Both sides must fault on the first write so
that the writer gets a private copy. The refcount-1 shortcut saves a
frame allocation and a 4 KiB copy: after the other side has copied (or
exited), the page belongs to one address space only. Read-only pages can
never be written by anyone, so sharing them forever is correct and
nothing needs copying. That is how all processes share one copy of the
program's code.

**Q6.** The frame comes from the free pool and may contain the last
owner's data: passwords, keys, file contents. Not zeroing it is an
information leak between processes, and between kernel and user. Other
places: page tables (must be zeroed for *correctness*, since junk entries
look present). Kernel heap memory (`kmalloc`, SWEB's `new`) copied to user
space: a struct with padding bytes or unset fields sent to user space by
a system call leaks old kernel heap contents. Pages given to user space by
a later `read()` when the file is shorter than the buffer.

**Q7.** With 3 frames FIFO makes 9 faults, with 4 frames 10. Having more
frames changes *which* page is the oldest at each point. With 4 frames,
pages 1 and 2 are evicted exactly before they are needed again (at the
second `1 2 3 4 5`). LRU and OPT are **stack algorithms**: at every step,
the set of pages in memory with k frames is a subset of the set with k+1
frames. The k+1 set is always "the k most recent/useful pages plus one".
So every hit with k frames is also a hit with k+1 frames. FIFO's set with
more frames can contain different pages entirely.

**Q8.** R = "this page was accessed since the hand last passed". On x86,
the MMU sets the **accessed** bit (bit 5) in the page-table entry on
every access. The kernel reads and clears it; `PageTableEntry` in SWEB
has the field, next to `dirty` (set on writes: the page must be written
back before eviction). If every bit is set, the hand clears them all in
one round and takes the frame it started with, which is exactly **FIFO**.

**Q9.** Below the main stack, one slot per thread, e.g. thread *i* gets
the pages `[USER_BREAK − (i+1)·S, USER_BREAK − i·S − G)` with S = slot
size and G = one unmapped guard page. The size should be generous in
*virtual* space (a few hundred KiB up to MiBs), because pages are only
allocated on first touch. The guard page turns a stack overflow into a
clean fault instead of silently overwriting the neighbouring thread's
stack. Today, `PageFaultHandler::checkPageFaultIsValid` accepts any user
address, and `Loader::loadPage` searches the ELF segments. A stack
address belongs to none, so the process is killed ("No section refers to
the given address", `Syscall::exit(666)`). The fault handler needs to know
the thread stack regions (like `as_add_region`) and allocate zeroed pages
there instead of asking the loader. Alternatively, map a fixed number of
stack pages eagerly in `pthread_create`. A slot must be reusable after the
thread has been joined.

**Q10.** (a) Same page: both threads allocate a frame, both fill it from
the binary, both call `mapPage`. The second sees the PTE already present,
gets `false` and frees its frame. That is the case the "mapped by someone
else" branch handles, assuming both `mapPage` calls do not interleave
inside. (b) `mapPage` is not atomic. Both threads can find `pt_ppn == 0`
(or `pd_ppn`/`pdpt_ppn`), both allocate a new table, and both `insert` it
into the same upper-level entry. The second write wins. The first table
leaks, together with the page just mapped through it, which is
silently lost. The next access faults again, or data written there
disappears. The two threads also race on the PageManager and on the
debug-printed state. The fix is a lock per address space (in
`ArchMemory`/`Loader`), held from the page walk to the final `insert` in
`mapPage`/`unmapPage` and in the page-fault path. With threads in one
address space, which your A1 work creates, this lock becomes necessary.
