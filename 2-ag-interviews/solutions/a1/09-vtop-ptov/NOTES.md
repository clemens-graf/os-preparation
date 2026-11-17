# A1-09 vtop / ptov – solution notes

Patch: [`solution.patch`](solution.patch).

```cpp
size_t Syscall::vtop(size_t address)
{
  ScopeLock lock(process->getProcessLock());
  ArchMemoryMapping m = process->getAddressSpace().resolveMapping(address / PAGE_SIZE);
  return m.page_size == PAGE_SIZE ? m.page_ppn : (size_t) -1;
}
```

`ptov`: `ArchMemory::findVirtualPage(ppn)` – the 4-level walk of the user half; the indices give
the vpn: `((pml4i * 512 + pdpti) * 512 + pdi) * 512 + pti`.

## Notes

- Lazy loading: `vtop(&global)` is -1 until the first access – the test found exactly that.
- There is no reverse mapping in SWEB, hence the walk. Real kernels keep one (Linux: rmap), because
  swapping and page migration (A2!) need "who maps this frame?".
- ppn validity: `ppn < getTotalNumPages()`.

## Shortest working version (tested)

[`short.patch`](short.patch) – **60 lines** instead of 85, passes the same test. Get it with
`task short a1-09` (branch `short/…`, then `task test`).

What is shorter: both syscalls inline in their `case`; no ppn range check (the search simply finds nothing).

Use it to see the **core** of the task. In the interview, the full solution's extra checks are what
a tutor asks about ("what if the pointer is invalid / two threads at once?").
