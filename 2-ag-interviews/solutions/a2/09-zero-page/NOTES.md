# A2-09 zero-page deduplication + copy-on-write – solution notes

Patch: [`solution.patch`](solution.patch).

## The pieces

```cpp
// ArchMemory: one page of the KERNEL IMAGE, so the PageManager never counts it
static char shared_zero_page[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
void ArchMemory::initSharedZeroPage()        // from startup()
{
  vpn = (size_t) shared_zero_page / PAGE_SIZE;
  page_size = get_PPN_Of_VPN_In_KernelMapping(vpn, &ppn);
  shared_zero_ppn_ = ppn * (page_size / PAGE_SIZE) + vpn % (page_size / PAGE_SIZE);  // 2 MiB pages!
}
// unmapPage, ~ArchMemory: never freePPN(shared_zero_ppn_)

// Loader::loadPage: no file content on this page -> map the zero page read-only
// PageFaultHandler, BEFORE checkPageFaultIsValid:
if (present && writing && address < USER_BREAK && loader && loader->copyOnWrite(vpn))
  return;

bool Loader::copyOnWrite(size_t vpn)          // under arch_memory_lock_
{
  if (pte.page_ppn != shared_zero_ppn_) return pte.writeable;   // someone else copied it already
  pte.page_ppn = PageManager::instance()->allocPPN();           // zeroed = the copy
  pte.writeable = 1;
  ArchMemory::flushTlb();                                       // PPN of a present mapping changed
  return true;
}
```

## The things that bite

- **The kernel writes too.** CR0.WP is set (`boot.32.C`): a syscall writing into a read-only user
  page faults in kernel mode. Without WP the kernel would silently write into the shared zero page –
  every process would see the garbage. The COW branch therefore does not check `user`.
- **Present faults never reach the valid branch** – COW must come before `checkPageFaultIsValid`.
- **The TLB** may still map vpn → zero page: flush after changing the PPN.
- **The leak check**: my first version took the zero page from the PageManager and never freed it –
  SWEB's shutdown check reported exactly one leaked page. A page of the kernel image is not counted.
- Two threads write the same zero page: the second finds `page_ppn != zero` under the lock and just
  returns (retry).

## Generalisation: fork's COW (your A2 mandatory part)

Instead of one special page: every page of the parent becomes read-only in both processes and gets
a **reference count**; the write fault copies (memcpy via the ident mapping), decrements the count,
and the last owner just gets its page writable again. Same fault path, same locking questions – plus
two processes' page tables.
