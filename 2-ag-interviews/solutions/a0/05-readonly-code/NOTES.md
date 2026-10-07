# A0-05 read-only code – solution notes

Patch: [`solution.patch`](solution.patch).

## Files touched

Only `common/source/kernel/Loader.cpp` (+ the page-table lock in `Loader.h`).

## The code

```cpp
#define ELF_PF_W 2
...
bool found_page_content = false;
bool writable = false;
for (each phdr)
{
  if (phdr.p_vaddr < page_end)
  {
    if (phdr.p_vaddr + phdr.p_filesz > page_start)      // file content on this page
    {
      ... copy ...
      found_page_content = true;
      writable |= (phdr.p_flags & ELF_PF_W) != 0;
    }
    else if (phdr.p_vaddr + phdr.p_memsz > page_start)  // only .bss on this page
    {
      found_page_content = true;
      writable |= (phdr.p_flags & ELF_PF_W) != 0;
    }
  }
}
...
arch_memory_lock_.acquire();
bool page_mapped = arch_memory_.mapPage(vpn, ppn, true);
if (page_mapped && !writable)
{
  ArchMemoryMapping m = arch_memory_.resolveMapping(vpn);
  m.pt[m.pti].writeable = 0;
}
arch_memory_lock_.release();
```

- **Any** writable segment on the page makes the page writable. Segments of SWEB programs
  are page-aligned (`readelf -lW`: `.text` R E at 0x8000000, `.rodata` R at 0x8004000,
  `.data/.bss` RW at 0x8005000), but in general a page can hold the end of one segment and the
  start of the next – then the stricter choice would break the writable one.
- `p_filesz` = bytes in the file, `p_memsz` = bytes in memory; the difference is `.bss`
  (zero-initialised, not stored in the file). The second branch catches pages that hold only
  `.bss`.

## Locking

Map + clear the bit in one critical section (see A0-04). `phdrs_` is read under
`program_binary_lock_` (already taken by `loadPage` for the file reads).

## Answers to the tutor questions

- *Write to `main`:* #PF present + write + user → `checkPageFaultIsValid` → exit 9999.
- *Non-executable stack/heap:* set `execution_disabled` (NX) in the PTE – SWEB enables
  `EFER.NXE` at boot (`boot.32.C`), so the bit works. Instruction fetch from such a page
  faults with the *instruction fetch* flag set. That is an A2 elective (W⊕X).
