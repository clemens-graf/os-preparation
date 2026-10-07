# A0-12 · two addresses, one page (tutorial 2 done right)

**Time box 40 min · ★★★ · read first: [ex12](../../examples/ex12-tutorial2/) (the tutorial), [ex02](../../examples/ex02-map-page/)**

```
Name:         get_physical_address, map_address
Nr:           1560 (sc_get_physical_address), 1561 (sc_map_address)
Description:  get_physical_address(address) returns the physical page number (ppn) of the
              page that contains address in the calling process.
              map_address(address, ppn) maps the page that contains address to physical
              page ppn (user-accessible, writable): the same memory is then visible at two
              addresses. A process may only map physical pages that are already mapped in
              its own address space.
Parameters:   size_t address   any address inside the page
              size_t ppn       (map_address) a physical page number
Return Value: get_physical_address: the ppn, or -1 if address is not a user address or its
              page is not mapped (yet).
              map_address: 0, or -1 if address is not a user address, its page is already
              mapped, or ppn is not mapped in the calling process.
Notes:        The process must be able to exit afterwards: no kernel panic, no frame freed
              twice, no leak. Threads of the process may call both syscalls at the same time.
```

This is the tutorial from 7 Oct (ex12) with every gap closed: the tutorial version lets a
process map any frame (also the kernel's), crashes the kernel through an `assert`, returns 0
for "not mapped", and panics after every run because a frame mapped twice is freed twice.

## Test

`task start a0-12`, implement, `task test`.
Done when all 23 checks pass and the final `exit` reports neither a kernel panic nor a leak.
`alias_basic.sweb` is the tutorial scenario (two aliases of a global, one of the stack),
`alias_errors.sweb` everything `map_address` must refuse, `mult.sweb` + `exit` show that the
kernel survived and freed every frame exactly once.

## Hints

<details><summary>1 – start from the tutorial</summary>

ex12's two functions are the skeleton (same six places, numbers 1560/1561). Then fix them one
by one: `address >= USER_BREAK` → `-1`; replace `assert(mapped)` by a return value; return
`(size_t) -1`, never `-1U`. `currentThread->loader_` is enough – no cast to `UserProcess`.
</details>

<details><summary>2 – get_physical_address: when is a page mapped?</summary>

`ArchMemoryMapping m = arch_memory_.resolveMapping(address / PAGE_SIZE);` –
`m.page_size == PAGE_SIZE` means a 4 KiB page is mapped, then `m.page_ppn` is its frame.
Otherwise `page_ppn` is 0, which is a *real* frame number – answer `-1`.
</details>

<details><summary>3 – is this frame mine?</summary>

There is no reverse map (frame → virtual pages) in SWEB. Walk every present PTE of the user
half – the four loops of `ArchMemory::~ArchMemory()`, PML4 index `0 .. 255` – and compare
`page_ppn`. Put the walk into `ArchMemory` (it knows `page_map_level_4_`). The check and the
`mapPage` belong into **one** critical section.
</details>

<details><summary>4 – the double free at exit</summary>

`~ArchMemory` frees every present user page – it assumes each frame is mapped once. Remember
every alias vpn (a list in the `Loader`), and unmap them **without freeing** before
`arch_memory_` is destroyed. `Loader::~Loader()` runs before its members are destroyed – the
right place. `unmapPage` always frees: give it a parameter
`bool free_physical_page = true` (A1-10 does the same for shared pages).
</details>

<details><summary>5 – locking</summary>

The page-table lock from ex02: `Mutex arch_memory_lock_` in the `Loader` (initialised first:
`-Werror=reorder`), also around the `mapPage` in `Loader::loadPage`. It protects the page
tables *and* your alias list. Not needed in `~Loader`: no thread of the process is left.
</details>

## Questions a tutor might ask

- Why did the tutorial version panic at exit, and why does the backtrace name the CleanupThread?
- Why is 0 a bad answer for "not mapped"? Why does `get_physical_address(&global)` fail before
  the first access to `global`?
- What could a process do if `map_address` accepted any ppn? Show it with
  `get_physical_address(0xffffffff80100000)` of the tutorial version.
- Why must the ownership check and `mapPage` be one critical section?
- Explain `(vpn << 12) + (address & 0xfff)`. Why is the offset the same in the virtual and the
  physical address?
- No TLB flush after `mapPage` – why is that correct? Do you need one in `~Loader`?
- Which other syscall would break your solution? (One that unmaps **and frees** a page, like
  A0-02's `freepage`: the alias would keep a freed frame. What would you need then?)
