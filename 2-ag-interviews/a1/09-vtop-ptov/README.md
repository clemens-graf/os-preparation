# A1-09 · vtop / ptov

**Time box 25 min · ★★☆**

**Theory first:** [concepts.pdf](../../concepts.pdf) §16 frames, shared pages, §1 address space

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

## Your team's code

`process->getAddressSpace().resolveMapping(vpn)` under `process->getThreadLock()`; `ptov` needs a new walk `ArchMemory::findVirtualPage(ppn)` (a copy of the destructor's loops, `public:` in `ArchMemory.h`).

## Hints

<details><summary>1 – vtop</summary>

`resolveMapping(address / PAGE_SIZE)`, under the page-table lock; `page_size == PAGE_SIZE` → `page_ppn`.
</details>

<details><summary>2 – ptov</summary>

There is no reverse map: walk all present PTEs of the user half (like `~ArchMemory`). The four
indices *are* the virtual page number: `((pml4i*512 + pdpti)*512 + pdi)*512 + pti`.
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why can't the kernel answer ptov fast? How do real kernels do it (rmap)?</summary>

Page tables are a tree from vpn to ppn – the other direction has no index, so `ptov` must search all present entries of the process. Real kernels keep a **reverse map** (Linux: `rmap` – per physical page a list of the page tables / VMAs that map it, kept up to date on every map/unmap), which makes the lookup cheap but every mapping change more expensive.

</details>

<details><summary>Why does vtop of a global variable fail before the first access?</summary>

**Lazy loading:** the page of the global is mapped only by the page fault of the first access. Before that its PTE is not present → `resolveMapping` gives `page_size = 0` → `vtop` returns -1.

</details>

<details><summary>Can two virtual pages map the same ppn? (shared memory, zero page in A2)</summary>

Yes – nothing in the page tables forbids it: shared memory (A1-10, the same frame in two processes), aliases (A0-12, two addresses in one process), the shared zero page (A2-09), copy-on-write after fork. That is why `ptov` must define *which* vpn (the lowest) – and why such frames need a reference count before they may be freed.

</details>
