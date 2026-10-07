# A2-03 swap onto the kernel heap – solution notes

Patches: [`on-top-of-a2-01.patch`](on-top-of-a2-01.patch), [`solution.patch`](solution.patch).

Only `SwapManager` changes – that is the point of the task: everything above it (PTE marker, page
fault, exit) does not care where a slot lives.

```cpp
char* buffers_[HEAP_SWAP_SLOTS];   // nullptr = free, protected by lock_
writeOut: find a free index, buffers_[i] = new char[PAGE_SIZE], memcpy from the ident address
readIn:   memcpy to the ident address of the new page
freeSlot: delete[] buffers_[slot]; buffers_[slot] = nullptr
```

- 64 pages = 256 KiB of kernel heap – the limit keeps the kernel itself from running out (`new` in
  SWEB does not return null: the KernelMemoryManager asserts).
- `new` under `lock_` is fine: a Mutex, and the KMM has its own (Spin)lock – lock order SwapManager →
  KMM, never reversed.
- Why it is pointless in practice: swapping exists because RAM is full – putting pages into RAM
  (the kernel heap is RAM) only moves the problem. It makes a good exercise because it isolates the
  bookkeeping from the device.
