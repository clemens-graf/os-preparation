# A0-02 · allocpage / freepage

**Time box 30 min · ★★☆ · read first: [ex02](../../examples/ex02-map-page/)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §16 frames, shared pages, §4 page faults

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

## Your team's code

The task starts from your team's `main` (`osw26e3`, the commit in `team/BASE`) – the code you have in the interview.

- `UserProcess* process = currentThread->getProcess();` – then `process->getAddressSpace()`
  (`mapPage`, `resolveMapping`, `unmapPage`) and `process->getThreadLock()` around search + map.
- 1510/1511 are free in your team's syscall list.


## Hints

<details><summary>1 – the building blocks</summary>

`PageManager::instance()->allocPPN()` (a zeroed physical page),
`process->getAddressSpace().mapPage(vpn, ppn, user_access)`, `….resolveMapping(vpn)`
(`.page_size != 0` means mapped), `….unmapPage(vpn)`. Note: `mapPage` takes a
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

Answer each one out loud first – then open it to compare.

<details><summary>What happens to the physical page in <code>freepage</code>? Who frees the page-table pages?</summary>

`unmapPage(vpn)` frees the physical page itself (`freePPN`) – don't free it again ("Double free PPN"). Page-table pages that become empty are freed by `unmapPage` too (`checkAndRemove`); the rest go when the process exits (`~ArchMemory`).

</details>

<details><summary>Why is the TLB flush needed? Would your test notice a missing flush?</summary>

The CPU caches translations in the **TLB**. After `unmapPage` the old entry may still be cached, so the process could keep using a frame that is already free – and maybe handed to someone else. `flushTlb()` (reloads `cr3`) drops them. The test usually doesn't notice a missing flush on a single CPU – a context switch to another process flushes too – which is exactly why you must argue, not test.

</details>

<details><summary>What does <code>mapPage</code> return if the slot is already mapped, and what must the caller do then?</summary>

`false` – nothing was changed. The caller still owns the `ppn` it allocated and must give it back (`freePPN`) or use another slot. Ignoring it = a leak of one frame.

</details>

<details><summary>Your lock protects what exactly? Show the critical section.</summary>

`thread_lock_` protects the page tables: **"find a free slot" and "map it" in one critical section**. Otherwise two threads both see slot 7 free, both map it – one `mapPage` fails, but its caller already returned slot 7 as its own. `allocPPN` can stay outside (keeps the section short).

</details>

<details><summary>What happens to pages the program never freed when it exits?</summary>

Nothing special is needed: at exit `~ArchMemory` walks the whole lower half and frees every present page and every page-table page. The leak check (`exit` at the end of `task test`) would report anything that stays.

</details>
