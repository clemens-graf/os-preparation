# A2-09 · zero-page deduplication + copy-on-write

**Time box 45 min · ★★★**

```
Name:         deduplication zero
Nr:           1770 (freepages, for testing)
Description:  Pages that would be all zero (pure .bss pages) are not given their own
              physical page: they all map ONE shared zero page, read-only. The first write
              to such a page (from user OR kernel mode) gives the process its own copy
              (copy-on-write), writable. The shared zero page is never freed.
              freepages() returns the number of free physical pages.
```

## Test

`task.sh start a2-09` → `task.sh test`. Done when reading 64 zero pages costs (almost) no memory,
writing 32 of them costs about 32 pages, a syscall writing into a zero page works, the zero page
stays zero, and the shutdown leak check is clean.

## Hints

<details><summary>1 – which pages are 'zero'?</summary>

In `Loader::loadPage`: a page that got no bytes from the file (only `.bss`).
</details>

<details><summary>2 – the copy-on-write fault</summary>

A write to a present read-only page: `present = 1`, `writing = 1`. `checkPageFaultIsValid` rejects
those – your COW branch must come *before* it. Under the page-table lock: if the PTE points to the
zero page → new page, writable, TLB flush (the PPN of a present mapping changed).
</details>

<details><summary>3 – the kernel writes too</summary>

CR0.WP is set in SWEB (`boot.32.C`: `or $0x80010001, cr0`): a syscall writing into a read-only user
page faults as well – so the COW branch must not require `user`.
</details>

<details><summary>4 – never free it</summary>

`unmapPage` and `~ArchMemory` free every present page – skip the zero page. Even better: take it from
the kernel image (a page-aligned static array), not from the PageManager – then SWEB's leak check at
shutdown does not count it.
</details>

## Questions a tutor might ask

- Without CR0.WP, what would a read() into a fresh .bss buffer do?
- Why the TLB flush after the copy?
- How does this generalise to fork's copy-on-write (reference counts)?
