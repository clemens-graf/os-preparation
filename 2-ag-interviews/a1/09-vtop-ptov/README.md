# A1-09 · vtop / ptov

**Time box 25 min · ★★☆**

**Theory first:** [concepts.pdf](../../concepts.pdf) §16 frames, shared pages, §1 address space

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

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

## Level 2 – hints

<details><summary>1 – vtop</summary>

`resolveMapping(address / PAGE_SIZE)`, under the page-table lock; `page_size == PAGE_SIZE` → `page_ppn`.
</details>

<details><summary>2 – ptov</summary>

There is no reverse map: walk all present PTEs of the user half (like `~ArchMemory`). The four
indices *are* the virtual page number: `((pml4i*512 + pdpti)*512 + pdi)*512 + pti`.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Syscall plumbing for two syscalls (1670 `vtop`, 1671 `ptov`): numbers, declarations in `Syscall.h` (take the address as `size_t`), two `case`s, the libc wrappers in `nonstd.h/.c` (`ssize_t vtop(void*)`, `void* ptov(size_t)`).
2. `vtop`: refuse addresses `>= USER_BREAK` with -1.
3. Take the process's `thread_lock_`, call `resolveMapping` with the **page number** (address / page size).
4. If the result's page size is not one page, the page is not mapped: return -1. Otherwise return its `page_ppn`.
5. `ptov`: add a public method to `ArchMemory` that searches the page tables: copy the four nested loops of `~ArchMemory` (lower half only), but **only read** – no `present = 0`, no freeing.
6. In the innermost loop: present and the same ppn → rebuild the vpn from the four indices (each level multiplies by 512) and return it. After all loops: return "not found" (-1).
7. In the syscall: take `thread_lock_`, call the search; not found → return 0 (NULL), else return vpn × page size.
8. `task test`: 8 checks, including "a page that was never touched has no ppn yet".

</details>

## Your team's code

`process->getAddressSpace().resolveMapping(vpn)` under `process->getThreadLock()`; `ptov` needs a new walk `ArchMemory::findVirtualPage(ppn)` (a copy of the destructor's loops, `public:` in `ArchMemory.h`).

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
