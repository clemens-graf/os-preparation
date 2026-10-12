# A0-05 read-only code – solution notes

Patch: [`solution.patch`](solution.patch) – for your team's code (`task solution`).
Base SWEB (`task mode base`): [`base-sweb/solution.patch`](base-sweb/solution.patch).

## Files touched

`common/source/kernel/Loader.cpp` (which segments are writable) and `ArchMemory::mapPage` (a parameter `bool writeable = true`).

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
// your team's loadPage gets the address space as a parameter and runs WITHOUT thread_lock_:
// map read-only in ONE step - no moment in which another thread could see the page writable
bool page_mapped = arch_memory_.mapPage(vpn, ppn, true, writable);
```

- **Any** writable segment on the page makes the page writable. Segments of SWEB programs
  are page-aligned (`readelf -lW`: `.text` R E at 0x8000000, `.rodata` R at 0x8004000,
  `.data/.bss` RW at 0x8005000), but in general a page can hold the end of one segment and the
  start of the next – then the stricter choice would break the writable one.
- `p_filesz` = bytes in the file, `p_memsz` = bytes in memory; the difference is `.bss`
  (zero-initialised, not stored in the file). The second branch catches pages that hold only
  `.bss`.

## Locking

No moment in which another thread sees the page writable: `mapPage(..., writable)` creates the
entry with the right bit in one step (base SWEB: map + clear the bit inside one critical section
of the page-table lock). `phdrs_` is read under `program_binary_lock_` (already taken by
`loadPage` for the file reads).

## Answers to the tutor questions

- *Write to `main`:* #PF present + write + user → `checkPageFaultIsValid` → exit 9999.
- *Non-executable stack/heap:* set `execution_disabled` (NX) in the PTE – SWEB enables
  `EFER.NXE` at boot (`boot.32.C`), so the bit works. Instruction fetch from such a page
  faults with the *instruction fetch* flag set. That is an A2 elective (W⊕X).
