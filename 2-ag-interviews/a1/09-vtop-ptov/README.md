# A1-09 · vtop / ptov

**Time box 25 min · ★★☆**

```
Name:         vtop, ptov
Nr:           1670, 1671
Description:  vtop(address) returns the physical page number that the page of address
              is mapped to in the calling process (-1 if not mapped).
              ptov(ppn) returns the lowest virtual address in the calling process whose
              page is mapped to physical page ppn (NULL if none).
Parameters:   void* address / size_t ppn
```

## Test

`task.sh start a1-09` → `task.sh test`. Done when 8 checks pass. Note: a page that was never
touched has no physical page yet (lazy loading) – the test checks that too.

## Hints

<details><summary>1 – vtop</summary>

`resolveMapping(address / PAGE_SIZE)`, under the page-table lock; `page_size == PAGE_SIZE` → `page_ppn`.
</details>

<details><summary>2 – ptov</summary>

There is no reverse map: walk all present PTEs of the user half (like `~ArchMemory`). The four
indices *are* the virtual page number: `((pml4i*512 + pdpti)*512 + pdi)*512 + pti`.
</details>

## Questions a tutor might ask

- Why can't the kernel answer ptov fast? How do real kernels do it (rmap)?
- Why does vtop of a global variable fail before the first access?
- Can two virtual pages map the same ppn? (shared memory, zero page in A2)
