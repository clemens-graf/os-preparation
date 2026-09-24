# Module 08 — Paper questions

Answers: `../../solutions/08-virtual-memory/answers.md`.
Paths refer to your SWEB repo (`repos/osw26e3`).

**Q1.** Split the virtual address `0x00007f1234567abc` into its four
page-table indices and the page offset. How many page-table frames does
an empty address space need to map (a) one page at `0x400000`, (b) 1 GiB
of contiguous memory starting at `0x40000000`?

**Q2.** Why four levels? How big would a single flat page table for a
48-bit address space be (8-byte entries, 4 KiB pages)? What does the
multi-level design cost on every memory access, and what hides that cost?

**Q3.** SWEB's `ArchMemory::mapPage` inserts the upper-level entries with
`user_access = 1` and `writeable = 1` although the page itself may be
read-only or kernel-only. Why is that safe? What would happen if a PML4
entry had `user_access = 0`?

**Q4.** (a) Why must `vm_unmap` invalidate the TLB, but `vm_map` of a
page that was not present need not? (b) On a machine with several CPUs,
another CPU may have the old translation cached. What is a *TLB
shootdown*, and why is it expensive? (c) In `as_fork`, why is the parent's
TLB a problem at all — the parent's pages only become *more* restricted?

**Q5.** Copy-on-write: why must `fork` make the *parent's* writable pages
read-only too? What does the "refcount is 1 → just make it writable"
shortcut save? Why are read-only pages (code) shared without the COW mark?

**Q6.** Why must a lazily allocated page be zeroed? Name another place in
a kernel where memory that someone else used before can leak to a user
process.

**Q7.** Explain Belady's anomaly with the FIFO example from the tests
(`1 2 3 4 1 2 5 1 2 3 4 5`, 3 vs. 4 frames). Why can LRU and OPT never
show it?

**Q8.** Clock approximates LRU. What does the reference bit mean, who
sets it on x86 (look at the page-table entry structs in
`arch/x86/64/include/paging-definitions.h`), and what does Clock
degenerate to when every bit is set?

**Q9.** (P1) Every thread needs its own user stack. In SWEB, the first
thread's stack is one page, mapped in `UserProcess.cpp`. Where would you
place the stacks of threads created by `pthread_create`? How large, and
why a gap between them? What happens *today* if a thread touches an
address just below its stack page (follow `PageFaultHandler` →
`Loader::loadPage`)? What must change?

**Q10.** (P1) After A1, two threads of one process can page-fault at the
same time. Walk through `Loader::loadPage` and `ArchMemory::mapPage` for
two threads faulting (a) on the same page, (b) on two different pages
whose page table does not exist yet. Which race does the "has been
mapped by someone else" branch handle, and which one does it miss?
