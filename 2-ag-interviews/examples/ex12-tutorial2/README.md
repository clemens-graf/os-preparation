# ex12 – tutorial 2: one physical page at two virtual addresses

The change from the A0 preparation tutorial (7 Oct), **exactly as it was written there**
(`example.patch` = the tutorial's `tutorial2.patch` without the test program). Two syscalls:

- `get_physical_address(va)` – the physical page number (ppn) behind virtual address `va`;
- `map_address(va, ppn)` – map the page that contains `va` to physical page `ppn`.

`tutorial2.c` is the tutorial's program, unchanged. `ex_tutorial2.c` does the same with
`[PASS]`/`[FAIL]` checks. Both end the same way: the program finishes correctly, and then
**the kernel panics** – that is part of the lesson, see below.

```bash
task example ex12      # branch example/ex12-tutorial2 in repos/sweb-ag
task test              # 5 checks pass, then the expected "Double free PPN" panic
task run               # boot it, type tutorial2.sweb (the original) or ex_tutorial2.sweb
```

`task test` runs with `--expect-panic`: here the panic is the expected result, so the test
counts it as success. [A0-12](../../a0/12-alias-page/) is the same task done right – no panic.

## The change: the usual six places

| Place | Line |
|---|---|
| `common/include/kernel/syscall-definitions.h` | `sc_get_physical_address 142`, `sc_map_address 143` |
| `common/include/kernel/Syscall.h` | the two static methods |
| `Syscall::syscallException` | two `case`s; `return_value = ...` only for the one that returns something |
| `Syscall.cpp` | the implementation (below) |
| `userspace/libc/include/nonstd.h` | prototypes – plus `#include "types.h"`, or `size_t` is unknown there |
| `userspace/libc/src/nonstd.c` | wrappers: `__syscall(sc_..., arg1, arg2, 0, 0, 0)` |

```cpp
size_t Syscall::get_physical_address(size_t virtual_address)
{
  UserProcess *process = (UserProcess*)currentThread;
  ArchMemoryMapping m = process->loader_->arch_memory_.resolveMapping(virtual_address >> 12);
  return m.page_ppn;
}

void Syscall::map_address(size_t virtual_address, size_t ppn)
{
  UserProcess *process = (UserProcess*)currentThread;
  bool mapped = process->loader_->arch_memory_.mapPage(virtual_address >> 12, ppn, 1);
  assert(mapped);
}
```

- `resolveMapping(vpn)` walks the four page-table levels of *this* process and fills an
  `ArchMemoryMapping`: `m.page_ppn` is the frame, `m.page_size` is `PAGE_SIZE` if a 4 KiB page
  is mapped and 0 if not, `m.pt[m.pti]` is the page-table entry itself.
- `mapPage(vpn, ppn, 1)` writes a present, **writable**, user-accessible entry (`1` =
  `user_access`) and allocates missing page tables on the way. It returns `false` if the vpn is
  already mapped.
- No TLB flush is needed after `mapPage`: the page was not present before, so the CPU cannot
  have a stale translation cached.

## Addresses and page numbers

A 4 KiB page has a 12-bit offset. For any address:

```
vpn     = address >> 12        (= address / PAGE_SIZE)       virtual page number
offset  = address & 0xfff      (= address % PAGE_SIZE)       position inside the page
address = (vpn << 12) + offset
```

The page table only translates the **page number**. The offset is copied through unchanged,
so it is the same in the virtual and the physical address:

```
virtual  0x0000000008006100  =  vpn 0x8006      | offset 0x100
                                      | page table
physical 0x0000000000407100  =  ppn 1031 (0x407) | offset 0x100
```

(Numbers from a real run: `global` at `0x8006100`, ppn 1031.) That is why the tutorial
computes the alias of `global` as `(vpn_of_0x424242424242 << 12) + (&global & 0xfff)`. The
kernel itself reaches any physical page through the identity mapping:
`ArchMemory::getIdentAddressOfPPN(ppn)` = `IDENT_MAPPING_START + ppn * PAGE_SIZE`.

Two details the program depends on:

- `global = 2;` comes **before** `get_physical_address(&global)`. SWEB loads pages lazily: an
  untouched page has no physical page yet, and the call would return 0.
- After `map_address`, the frame is mapped at two places in the same process. The compiler
  does not know that, so the checked version uses `volatile int*` for the alias. Here it would
  work without (`int*` may alias an `int`), but it is the honest way to say "this memory
  changes behind your back".

## Why the kernel panics at exit

```
KERNEL PANIC: Assertion page_usage_table_->getBit(p) && "Double free PPN" failed
              in common/source/mm/PageManager.cpp
=== Begin of backtrace for kernelthread <CleanupThread> ===
PageManager::freePPN(...)
```

The program is long finished by then. What happens:

1. `main` returns → `_start` calls `exit` → `Syscall::exit` → `currentThread->kill()`.
2. The **CleanupThread** deletes the dead thread: `~UserProcess` → `delete loader_` →
   `~Loader` → `~ArchMemory` (a member of the Loader).
3. `~ArchMemory` walks every present entry of the user half and calls
   `PageManager::freePPN(ppn)` for each. It assumes **every user frame is mapped exactly once**.
4. The frame of `global` is mapped twice – at `0x8006000` and at `0x424242424000` – so it is
   freed twice. The second `freePPN` finds the frame already free → assertion → panic.

That is also why the backtrace names the CleanupThread, not the program.

## What a tutor would ask about this code

The tutorial code shows the mechanism. In an interview, each of these would cost you points
or come up as a question:

| Problem | Consequence | Fix |
|---|---|---|
| frame mapped twice, freed twice at exit | kernel panic after every run | remember the alias mappings; unmap them **without** freeing before `~ArchMemory` runs (A0-12; same idea as A1-10's shared page) |
| `ppn` is not checked | a process can map **any** frame: another process's memory, the kernel's code (`get_physical_address(0xffffffff80100000)` happily returns 256), page tables – and write to it | only accept a ppn that is already mapped in the caller's own address space |
| `va` is not checked against `USER_BREAK` | `map_address(0xffffffff80100000, ppn)` reaches the kernel half of the page tables | `-1` for `va >= USER_BREAK` |
| `assert(mapped)` | a user program can crash the kernel on purpose: map an address that is already mapped (tested: `map_address` onto kernel text → `Assertion mapped failed`) | return `-1`; an `assert` is for kernel bugs, never for bad user input |
| `get_physical_address` returns `m.page_ppn` blindly | 0 for a page that is not mapped (yet) – but 0 is a real frame number; a kernel address returns the kernel's frame | `-1` unless `va < USER_BREAK` and `m.page_size == PAGE_SIZE` |
| no lock around `arch_memory_` | fine while a process has one thread; with threads (A1) two syscalls or page faults change the tables at once | `loader->arch_memory_lock_` (ex02), around the check **and** the `mapPage` |
| `(UserProcess*)currentThread` | unnecessary in base SWEB (`currentThread->loader_` exists); wrong in the team repo, where a thread is not a process | `currentThread->loader_` / the team repo's `getLoader()` |
| `*address = 1;` in `tutorial2.c` | writes byte `0x242` of the page that holds `global` – whatever variable lives there is overwritten | compute the alias of `global` first (as the tutorial does afterwards) |
| kernel `map_address` returns nothing, the libc wrapper returns `size_t` | the user always gets 0, also on failure | return `0` / `-1` |
| syscall numbers 142/143 | fine in base SWEB | in the team repo: your own block (1500–1999) |

## Try it

- Swap the two lines `global = 2;` and the `get_physical_address(&global)` call: which ppn do
  you get now?
- Map the alias, then let `ex_tutorial2` print `get_physical_address` of the alias *before*
  writing through it: is it already mapped?
- Comment out `map_address` and keep the write through the alias: what does the page fault
  handler do with `0x424242424100`?
