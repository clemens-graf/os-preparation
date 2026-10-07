# A0-02 · allocpage / freepage

**Time box 30 min · ★★☆ · read first: [ex02](../../examples/ex02-map-page/)**

```
Name:         allocpage, freepage
Nr:           1510 (sc_allocpage), 1511 (sc_freepage)
Description:  The region [0x500000000000, 0x500000000000 + 512 pages) belongs to these
              two syscalls.
              allocpage maps a new, zeroed page at the LOWEST free slot of the region
              and returns its address.
              freepage(page) unmaps a page of the region again (its physical page is
              freed). Afterwards any access to it must kill the process.
Parameters:   allocpage: none.  freepage: void* page
Return Value: allocpage: the address, or NULL if all 512 slots are in use
              freepage: 0, or -1 if page is not page-aligned, not inside the region,
              or not mapped
Notes:        Two threads calling allocpage at the same time must get different pages.
```

## Test

`task start a0-02`, implement, `task test`.
Done when all 10 checks pass and the last access to a freed page kills the process
(`No section refers to the given address` / exit code 666).

## Hints

<details><summary>1 – the building blocks</summary>

`PageManager::instance()->allocPPN()` (a zeroed physical page),
`arch_memory_.mapPage(vpn, ppn, user_access)`, `arch_memory_.resolveMapping(vpn)`
(`.page_size != 0` means mapped), `arch_memory_.unmapPage(vpn)`. Note: `mapPage` takes a
**page number** (address / `PAGE_SIZE`), not an address.
</details>

<details><summary>2 – what does unmapPage do, what does it not do?</summary>

It clears the entry, **frees the physical page** (do not free it a second time!) and frees
page-table pages that became empty. It does **not** flush the TLB – the CPU may still have
the old translation cached and let the process keep using the freed page. Call
`ArchMemory::flushTlb()` afterwards.
</details>

<details><summary>3 – locking: the race in allocpage</summary>

"Search the lowest free slot" and "map it" must be **one** critical section. If you
release the lock in between, two threads find the same free slot; one `mapPage` then
returns false and that thread returns an address that belongs to the other one.
Allocate the physical page *before* taking the lock (keeps the critical section short)
and give it back with `freePPN` if the region is full.
</details>

## Questions a tutor might ask

- What happens to the physical page in `freepage`? Who frees the page-table pages?
- Why is the TLB flush needed? Would your test notice a missing flush?
- What does `mapPage` return if the slot is already mapped, and what must the caller do then?
- Your lock protects what exactly? Show the critical section.
- What happens to pages the program never freed when it exits?
