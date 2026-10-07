# A0-01 · meminfo

**Time box 25 min · ★☆☆ · read first: [ex01](../../examples/ex01-syscalls/)**

```
Name:         meminfo
Nr:           1500 (sc_meminfo)
Description:  Fills a struct in user space with memory statistics:
              - total_pages:   number of physical pages in the system
              - free_pages:    number of free physical pages
              - process_pages: number of user pages currently mapped (present) in the
                               address space of the calling process
Parameters:   struct meminfo* info
Return Value: 0 on success, -1 if info is not a valid user pointer
Notes:        Declare the struct in nonstd.h:
                struct meminfo { size_t total_pages; size_t free_pages; size_t process_pages; };
              The whole struct must lie in user space (below USER_BREAK).
              Count process_pages by walking the page tables of the process.
```

## Test

`task start a0-01`, implement, `task test` (runs `meminfo_basic.sweb`, then `exit` for the leak check).

Done when all 7 checks print `[PASS]`. Note: pages are loaded lazily – right after the
start only the code page and the stack page are mapped, so `process_pages` is small.

## Hints

<details><summary>1 – which files?</summary>

The six places of every new syscall: `syscall-definitions.h` (number), `Syscall.h`
(declaration), `Syscall.cpp` (case in the switch + implementation), `nonstd.h` (user
declaration + struct), `nonstd.c` (wrapper with `__syscall`), and the test. The free/total
counts come from `PageManager`. The page tables live in the process's `ArchMemory`
(`currentThread->loader_->arch_memory_`, arch-specific code in
`arch/x86/64/source/ArchMemory.cpp`).
</details>

<details><summary>2 – how do I walk the page tables?</summary>

Look at `ArchMemory::~ArchMemory()`: it visits every present entry of the lower half
(user space) on all four levels and frees it. Your walk is the same loop, but it counts
present PTEs instead of freeing them. Only the lower half of the PML4 belongs to user
space (`PAGE_MAP_LEVEL_4_ENTRIES / 2`).
</details>

<details><summary>3 – locking</summary>

Another thread of the process (A1!) or a page fault could change the page tables while
you walk them – and `unmapPage` even frees page-table pages. Protect the page tables with
a per-process lock (the solutions add `Mutex arch_memory_lock_` to `Loader`, see ex02) and
hold it during the walk. Copy the result into user memory *after* releasing it.
</details>

<details><summary>4 – the pointer check</summary>

`info != 0` and `info + sizeof(struct) <= USER_BREAK` – written so that it cannot
overflow: `info >= USER_BREAK - sizeof(...)` → reject.
</details>

## Questions a tutor might ask

- Why can the kernel write to `info` directly, without copying through some special function?
- What happens if `info` points to an unmapped user page? To an address above `USER_BREAK`?
- Why is `process_pages` only 2 at the start of the program, although the binary is bigger?
- Which page-table entries did you *not* count (page-table pages themselves, kernel half)?
- Your walk takes a lock – which one, what does it protect, who else takes it?
