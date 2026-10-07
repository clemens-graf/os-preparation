# ex08 – the block device (swap partition)

`disknote(write, buffer)` writes/reads 512 bytes to/from the first block of `idea2`.
Test: `ex_block_device.sweb` (4 checks).

## The disk

The SWEB image has partitions `idea0` (boot), `idea1` (minixfs, mounted at `/usr`), and
`idea2` (type 0x82 "Linux swap", ~50 MB, unused in base SWEB – yours for A2).

```cpp
BDVirtualDevice* device = BDManager::getInstance()->getDeviceByName("idea2");
size_t block_size = device->getBlockSize();             // 512
device->writeData(offset_in_bytes, size_in_bytes, kernel_buffer);
device->readData(offset_in_bytes, size_in_bytes, kernel_buffer);
```

- Offset and size in **bytes**, multiples of the block size; returns the size or -1.
- The call **blocks** (the thread yields until the IDE interrupt reports completion): only
  with interrupts enabled, never while holding a SpinLock.
- Give the driver a **kernel buffer**, copy from/to the user buffer yourself.
- `run_test.sh` boots with `-snapshot`: writes are lost after the run. To test persistence
  across boots, boot the normal way (`sweb-practise`) twice.

## Global locks: no global constructors in SWEB!

```cpp
Mutex* Syscall::disk_note_lock_ = nullptr;
// main.cpp, startup(), before any thread that uses it exists:
Syscall::disk_note_lock_ = new Mutex("Syscall::disk_note_lock_");
```

The kernel never runs C++ constructors of global/static objects. A global
`Mutex lock("x");` is just zeroed memory – the first `acquire` asserts ("name_ is null,
maybe lock was not initialised?"). Create kernel-wide objects in `startup()` (or in a
singleton's `instance()` that is first called during boot).
