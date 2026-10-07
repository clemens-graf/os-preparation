# ex04 – change a page table entry: read-only pages

`protectpage(address, writable)` makes an existing mapping read-only or writable again.
Test: `ex_pte_flags.sweb` (4 checks, then the process must be killed by its write).

## The code

```cpp
ScopeLock lock(loader->arch_memory_lock_);
ArchMemoryMapping m = loader->arch_memory_.resolveMapping(address / PAGE_SIZE);
if (m.page_size != PAGE_SIZE)              // not mapped – m.pt may even be nullptr
  return (size_t) -1;
m.pt[m.pti].writeable = writable ? 1 : 0;
ArchMemory::flushTlb();
```

- `resolveMapping(vpn)` walks the 4 levels and returns pointers (ident addresses) to every
  level: `m.pml4`, `m.pdpt`, `m.pd`, `m.pt` and the indices `m.pti` etc. A level that does
  not exist is `nullptr`; `m.page_size` is `PAGE_SIZE` only if the 4 KiB page is present.
- The PTE fields (`paging-definitions.h`, `PageTableEntry`): `present`, `writeable`,
  `user_access`, `accessed`, `dirty`, `page_ppn`, `execution_disabled` (NX) – and
  `ignored_1` (11 bits) / `ignored_2` (3 bits) that the CPU ignores: free for your own
  flags (A2: "this page is swapped out, slot N").
- **TLB flush** after *removing* rights: the CPU may still have the old (writable) entry
  cached. Adding rights or mapping a non-present page needs no flush (a missing/weaker
  cached entry just faults once more).

## The write that kills

A write to a present read-only page raises a #PF with the *present* flag set →
`checkPageFaultIsValid`: "You got a pagefault even though the address is mapped" → exit 9999.
