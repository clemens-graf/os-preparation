# A1-09 vtop / ptov – solution notes

Patch: [`solution.patch`](solution.patch).

```cpp
size_t Syscall::vtop(size_t address)
{
  ScopeLock lock(loader->arch_memory_lock_);
  ArchMemoryMapping m = loader->arch_memory_.resolveMapping(address / PAGE_SIZE);
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
