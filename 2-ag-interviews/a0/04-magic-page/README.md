# A0-04 · the magic page

**Time box 25 min · ★★☆ · read first: [ex03](../../examples/ex03-stack-growth/), [ex04](../../examples/ex04-pte-flags/)**

```
Name:         magic page (no syscall)
Description:  The page at address 0xC0FFEE000 does not exist in any program. When a
              process accesses it for the first time, the page fault handler maps a new
              page there whose content is the string "coffee for <name of the thread>"
              (the rest of the page is zero).
              The page is READ-ONLY: every write to it kills the process – also when the
              very first access is a write.
Parameters:   -
Return Value: -
Notes:        Fill the page before the process can see it.
```

## Test

`task start a0-04`, implement, `task test` (runs `magic_page_basic.sweb` and `magic_page_write.sweb`).
Done when 3 checks pass and both programs are killed at their write (`pagefault even though the address is mapped`).

## Hints

<details><summary>1 – where and how do I fill the page?</summary>

In the page fault handler, before `loadPage`. Allocate a physical page and write the text
through the **ident mapping**: `ArchMemory::getIdentAddressOfPPN(ppn)` is a kernel address
that shows that physical page, no matter which page tables are active. `strcpy` is banned
in the kernel (it asserts) – use `memcpy`/`strncpy`.
</details>

<details><summary>2 – read-only</summary>

`mapPage` always maps writable. Afterwards `resolveMapping(vpn)` gives you `m.pt` and
`m.pti`; clear `m.pt[m.pti].writeable`. A write to a present, read-only page is a page fault
with the *present* flag set – `checkPageFaultIsValid` already treats that as an error.
</details>

<details><summary>3 – locking</summary>

Map and clear the writable bit in **one** critical section (the page-table lock). If
another thread of the process runs in between, it sees a writable magic page. And if two
threads fault on the page at the same time, the second `mapPage` returns false – free your
physical page then.
</details>

## Questions a tutor might ask

- What is the ident mapping and why can the kernel write to a page that is not mapped in user space?
- First access is a write – walk through both page faults.
- Why is no TLB flush needed after clearing the writable bit here, but in ex04 it is?
- How would you make the content different for every thread (e.g. the thread id)?
