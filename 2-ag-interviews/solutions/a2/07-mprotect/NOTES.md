# A2-07 mprotect + W⊕X – solution notes

Patch: [`solution.patch`](solution.patch).

```cpp
if (address % PAGE_SIZE || length == 0 || /* range beyond USER_BREAK */) return -1;
if ((prot & PROT_WRITE) && (prot & PROT_EXEC)) return -1;          // W xor X
ScopeLock lock(loader->arch_memory_lock_);
for (vpn in range) if (not present) return -1;                     // all or nothing: check first
for (vpn in range) {
  pte.user_access = prot != 0;                                     // PROT_NONE
  pte.writeable = prot & PROT_WRITE;
  pte.execution_disabled = !(prot & PROT_EXEC);                    // NX
}
ArchMemory::flushTlb();
```

- x86 page rights: present → readable; writable; NX. No "write-only", no "exec-only" without read.
- `PROT_NONE` via `user_access = 0`: the page stays present (no information lost), user accesses
  fault, the kernel could still use it.
- Lazy loading: untouched pages are not present – a full implementation would load them first
  (or store the rights and apply them in `loadPage`).
- Overflow-safe range check: number of pages ≤ `(USER_BREAK - address) / PAGE_SIZE`.
