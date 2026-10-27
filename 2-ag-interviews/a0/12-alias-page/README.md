# A0-12 · two addresses, one page (tutorial 2 done right)

**Time box 40 min · ★★★ · type S ([TRIAGE](../../TRIAGE.md)) · read first: [ex12](../../examples/ex12-tutorial2/) (the tutorial), [ex02](../../examples/ex02-map-page/)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §16 frames, shared pages

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

## Your team's code

The task starts from your team's `main` (`osw26e3`, the commit in `team/BASE`) – the code you have in the interview.

- **Never** copy the tutorial's `(UserProcess*) currentThread`: in your team's code `UserProcess`
  is no `Thread` – the cast compiles and reads garbage. Use `currentThread->getProcess()`, the page
  tables are `process->getAddressSpace()`, the lock `process->getThreadLock()`.
- The address space belongs to the `UserProcess`: `~UserProcess` does `delete arch_memory_`. So the
  alias list goes into `UserProcess` too (`addAlias(vpn)` under `thread_lock_`), and
  `~UserProcess` unmaps the aliases (`unmapPage(vpn, false)`) **right before** `delete arch_memory_`.
- 1560/1561 are free.


## Hints

<details><summary>1 – start from the tutorial</summary>

ex12's two functions are the skeleton (same six places, numbers 1560/1561). Then fix them one
by one: `address >= USER_BREAK` → `-1`; replace `assert(mapped)` by a return value; return
`(size_t) -1`, never `-1U`. Use `currentThread->getProcess()` – the tutorial's cast to
`UserProcess` is wrong in your team's code.
</details>

<details><summary>2 – get_physical_address: when is a page mapped?</summary>

`ArchMemoryMapping m = process->getAddressSpace().resolveMapping(address / PAGE_SIZE);` –
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
every alias vpn (a list in your `UserProcess`), and unmap them **without freeing** before
`arch_memory_` is destroyed. In `~UserProcess`, right before `delete arch_memory_` – the
right place. `unmapPage` always frees: give it a parameter
`bool free_physical_page = true` (A1-10 does the same for shared pages).
</details>

<details><summary>5 – locking</summary>

The page-table lock: your team's `process->getThreadLock()` – the page fault handler takes it
for stack pages too. Hold it around the ownership walk **and** `mapPage` (one critical section)
and while you add to the alias list. Not needed in `~UserProcess`: no thread of the process is
left.
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why did the tutorial version panic at exit, and why does the backtrace name the CleanupThread?</summary>

`~ArchMemory` frees **every** present user page. The aliased frame is present twice (original + alias) → `freePPN` twice → "Double free PPN" assertion. The CleanupThread runs the destructors (`~UserThread` → `~UserProcess` → `~ArchMemory`), so the panic happens there, not in the process.

</details>

<details><summary>Why is 0 a bad answer for "not mapped"? Why does <code>get_physical_address(&amp;global)</code> fail before the first access to <code>global</code>?</summary>

0 is a **valid** frame number – `resolveMapping` returns `page_ppn = 0` also for an unmapped page, so check `page_size == PAGE_SIZE` and return -1. `&global` fails before the first access because of **lazy loading**: the page is mapped only by the page fault of the first access.

</details>

<details><summary>What could a process do if <code>map_address</code> accepted any ppn? Show it with <code>get_physical_address(0xffffffff80100000)</code> of the tutorial version.</summary>

Map **any** physical frame into itself, writable: another process's memory, page tables, the kernel's code. `get_physical_address(0xffffffff80100000)` in the tutorial version walked into the kernel half and returned the kernel's frame – `map_address` then gives the process write access to the kernel.

</details>

<details><summary>Why must the ownership check and <code>mapPage</code> be one critical section?</summary>

Check-then-act (TOCTOU): between "this frame belongs to me" and `mapPage`, another thread could unmap and free the frame (e.g. A0-02's `freepage`) – the alias would then point to a frame that already belongs to someone else.

</details>

<details><summary>Explain <code>(vpn &lt;&lt; 12) + (address &amp; 0xfff)</code>. Why is the offset the same in the virtual and the physical address?</summary>

The page table translates only the **page number**: `vpn` → `ppn`. The low 12 bits (`address & 0xfff`) are the position inside the 4 KiB page and pass through unchanged – a page is 4096 bytes in both worlds. So `physical = (ppn << 12) + offset`, and the same for virtual with `vpn`.

</details>

<details><summary>No TLB flush after <code>mapPage</code> – why is that correct? Do you need one in <code>~UserProcess</code>?</summary>

After `mapPage`: the entry was **not present** before, and non-present entries are never cached in the TLB – nothing stale can exist. In `~UserProcess`: no, that address space never runs again (the CleanupThread runs on another `cr3`, `~ArchMemory` even asserts it).

</details>

<details><summary>Which other syscall would break your solution? (One that unmaps <b>and frees</b> a page, like A0-02's <code>freepage</code>: the alias would keep a freed frame. What would you need then?)</summary>

`freepage` would free a frame that is still mapped as an alias elsewhere → use after free. You'd need a **reference count** per frame (free only when the last mapping goes) or refuse to unmap a frame that has aliases – as A1-10 does for shared pages.

</details>
