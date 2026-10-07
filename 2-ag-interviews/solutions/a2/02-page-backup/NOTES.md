# A2-02 createPageBackup / loadPageBackup – solution notes

Patches: [`on-top-of-a2-01.patch`](on-top-of-a2-01.patch) (read this), [`solution.patch`](solution.patch).

- New "request types": `SwapManager::allocSlot()` and `writeSlot(slot, ppn)` (write a present page
  into a given slot, no unmapping).
- Per process: `PageBackup backups_[32]` (vpn → slot) under `arch_memory_lock_`.
- `createPageBackup(vpn)`: page must be present; existing entry → same slot (no extra disk space);
  else new entry + `allocSlot`; `writeSlot(slot, ppn)` **while holding the page-table lock**, so the
  page cannot be swapped out or unmapped during the write.
- `loadPageBackup(vpn)`: find the entry, page present → `readIn(slot, ppn)`: the disk content is
  written straight into the mapped physical page – the process sees 5 again.
- `~Loader` frees the backup slots.

Why "not on the stack": restoring a stack page restores old return addresses and saved registers
of functions that are running – the program jumps somewhere random.
