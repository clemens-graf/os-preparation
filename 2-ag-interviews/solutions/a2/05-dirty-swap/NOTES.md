# A2-05 don't write unchanged pages – solution notes

Patches: [`on-top-of-a2-01.patch`](on-top-of-a2-01.patch), [`solution.patch`](solution.patch).

## The idea

After swap-in the slot still holds exactly the page's content. Keep it, remember it in the
present entry (`ignored_1 = slot + 1`), clear `dirty`. Next swap-out:

```cpp
pte.present = 0;
ArchMemory::flushTlb();                       // from now on nobody can write -> dirty is final
if (pte.ignored_1 && !pte.dirty)      slot = pte.ignored_1 - 1;                 // no write at all
else if (pte.ignored_1)             { slot = pte.ignored_1 - 1; writeSlot(slot, ppn); }  // overwrite
else                                  slot = writeOut(ppn);                       // first time
pte.ignored_1 = 0;
```

## Details

- `ignored_1` has 11 bits → slot + 1 ≤ 2047: the device is limited to 2047 slots in this solution
  (alternative: a per-process vpn → slot table).
- Swap-in: `dirty = 0` is safe without a flush – the page was not present, so no TLB entry exists.
- The order: read `dirty` only after `present = 0` + flush. The CPU sets the dirty bit through the
  TLB entry on the first write; a write between "read dirty" and "present = 0" would be lost.
- Exit: present pages with `ignored_1` own a slot – `forEachSwappedPage` reports those too.
- The test counts device writes (`swapwrites`): first out = 1 write, clean out = 0, dirty out = 1
  into the same slot.
