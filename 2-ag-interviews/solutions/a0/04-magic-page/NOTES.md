# A0-04 magic page – solution notes

Patch: [`solution.patch`](solution.patch).

## Files touched

`PageFaultHandler.cpp` (`MAGIC_PAGE`, new branch), `Loader.h/.cpp` (`mapReadOnlyPage`,
page-table lock).

## The code

```cpp
if (address / PAGE_SIZE == MAGIC_PAGE / PAGE_SIZE)
{
  size_t ppn = PageManager::instance()->allocPPN();
  char* content = (char*) ArchMemory::getIdentAddressOfPPN(ppn);
  const char prefix[] = "coffee for ";
  memcpy(content, prefix, sizeof(prefix) - 1);
  strncpy(content + sizeof(prefix) - 1, currentThread->getName(), PAGE_SIZE - sizeof(prefix));
  if (!currentThread->loader_->mapReadOnlyPage(MAGIC_PAGE / PAGE_SIZE, ppn))
    PageManager::instance()->freePPN(ppn);
}

bool Loader::mapReadOnlyPage(size_t vpn, size_t ppn)
{
  ScopeLock lock(arch_memory_lock_);
  if (!arch_memory_.mapPage(vpn, ppn, 1))
    return false;
  ArchMemoryMapping m = arch_memory_.resolveMapping(vpn);
  m.pt[m.pti].writeable = 0;
  return true;
}
```

- **Ident mapping:** all physical memory is mapped into the kernel half at
  `IDENT_MAPPING_START + ppn * PAGE_SIZE`. `getIdentAddressOfPPN(ppn)` gives that address –
  the kernel can fill a page before it appears anywhere in user space.
- `strcpy` asserts in SWEB ("don't use strcpy"); `strncpy`/`memcpy` are fine.
- The page is filled **before** it is mapped: no thread can ever see it half-written.

## Locking

`mapPage` + clearing `writeable` in one critical section – otherwise another thread of
the process could write in between. No TLB flush: the entry was not present before, and
non-present entries are never cached.

## First access is a write

1. #PF not present, write → magic branch maps it read-only → return, write is repeated
2. #PF **present**, write → `checkPageFaultIsValid` → "pagefault even though the address
   is mapped" → `Syscall::exit(9999)`.

## Pitfalls

- Mapping with `mapPage` and forgetting the writable bit (page stays writable).
- Clearing the bit outside the lock.
- Not freeing the physical page when another thread was faster (`mapPage` false) – a leak
  the leak check at shutdown reports.
- `m.pt` is only valid if the page table exists – after a successful `mapPage` it does.
