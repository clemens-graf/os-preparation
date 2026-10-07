# A2-11 boot counter + message of the day – solution notes

Patch: [`solution.patch`](solution.patch).

```cpp
struct OnDisk { uint64 magic; uint64 boot_count; char motd[256]; char unused[...]; };  // 512 bytes
static_assert(sizeof(OnDisk) == 512);

BootRecord::BootRecord()                     // called from ProcessRegistry::Run()
{
  device_->readData(0, 512, (char*) &record_);
  if (record_.magic != MAGIC) { memset(&record_, 0, 512); record_.magic = MAGIC; }
  ++record_.boot_count;
  device_->writeData(0, 512, (char*) &record_);
  kprintf("This is boot number %zu. Message of the day: %s\n", ...);
}
```

- **When:** not in `startup()` – `readData` waits for the IDE interrupt by yielding, which needs a
  running thread with interrupts on. `ProcessRegistry::Run()` is the first kernel thread, before the
  first program.
- **Copying the message in:** copy at most 256 bytes into a kernel buffer, terminate it yourself –
  never `strlen` on user memory (it might not be terminated, or run into unmapped memory).
- **Lock:** one Mutex for the record and the block write: two processes calling `setmotd` must not
  interleave their copies and writes.
- **Sharing the partition with swap:** the swap slots must not start at offset 0 then – reserve the
  first page (slot 0) for the record.
- Testing persistence: `run_test.sh -n 2 --keep-disk` (both boots use one copy of the disk), or by
  hand: `make qemu` twice in the build folder without rebuilding (every `make` recreates the image).
