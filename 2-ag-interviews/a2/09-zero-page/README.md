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

## Your team's code

Your team's `loadPage` runs **without** the page-table lock – "map, then clear `writeable`" would leave a moment where every process could write into the shared zero page. The team solution gives `ArchMemory::mapPage` a `writeable` parameter (like A0-05) and maps read-only in one step. `copyOnWrite(vpn)` in `UserProcess` (under `thread_lock_`), called first in the page fault handler for present + writing faults.

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

Answer each one out loud first – then open it to compare.

<details><summary>Without CR0.WP, what would a read() into a fresh .bss buffer do?</summary>

`CR0.WP` makes the **kernel** respect read-only pages too. Without it, a `read()` syscall writing into a fresh `.bss` buffer (mapped read-only to the shared zero page) would **not fault** – the kernel would write straight into the shared zero page, and every process would suddenly see that data in its "zero" pages.

</details>

<details><summary>Why the TLB flush after the copy?</summary>

The PTE stays present but now points to a **new frame** – the TLB may still cache the old translation (→ the shared zero page, read-only). Without the flush the next access could still go to the zero page (and fault again, or read stale data). Any change of a present entry's frame or rights needs a flush.

</details>

<details><summary>How does this generalise to fork's copy-on-write (reference counts)?</summary>

`fork` shares **all** pages read-only between parent and child instead of copying them; a write faults, the faulting process gets its own copy, made writable. Unlike the zero page, these frames are shared by a varying number of processes – so each needs a **reference count**: copy on write while count > 1, the last owner just makes it writable again; free when the count reaches 0.

</details>
