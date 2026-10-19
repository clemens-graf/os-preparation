# ex09 – the loader: segments of the running program

`segments(out, max)` copies the loadable ELF segments (program headers) of the calling
process to user space. Test: `ex_loader.sweb` (3 checks; prints the segment list).

## What the example changes

| File | Lines |
|---|---|
| `common/include/kernel/Loader.h` | +15 |
| `common/include/kernel/Syscall.h` | +3 |
| `common/include/kernel/syscall-definitions.h` | +3 |
| `common/source/kernel/Loader.cpp` | +19 |
| `common/source/kernel/Syscall.cpp` | +20 |
| `userspace/libc/include/nonstd.h` | +12 |
| `userspace/libc/src/nonstd.c` | +5 |

The code below is the important part. The exact, complete change: `example.patch` in this folder, or `task example ex09` and then
`git -C ~/Documents/University/2026-fall/os/repos/sweb-ag show` (`q` quits).

## What the loader is

`Loader` (`common/source/kernel/Loader.cpp`), one per process: it owns the open binary
(`fd_`), the ELF header (`hdr_`), the list of loadable program headers (`phdrs_`, only
`PT_LOAD` survive `prepareHeaders`) and the **address space** (`arch_memory_`). Nothing is
copied into memory at start – `loadPage(address)` is called by the page fault handler for
every page on its first access and copies the right bytes from the file.

```
$ readelf -lW /tmp/sweb-ag/userspace/ex_loader.sweb
  LOAD 0x000000 0x0000000000400000 ... R      .note
  LOAD 0x001000 0x0000000008000000 ... R E    .text
  LOAD 0x005000 0x0000000008004000 ... R      .rodata .eh_frame
  LOAD 0x006000 0x0000000008005000 ... RW     .data .bss   (memsz > filesz: bss)
```

`p_flags`: 1 = PF_X, 2 = PF_W, 4 = PF_R.

## The code

The whole function from `example.patch` (`common/source/kernel/Loader.cpp`):

```cpp
size_t Loader::getSegments(SegmentInfo* out, size_t max)
{
  // phdrs_ is only written while the process is loaded, but loadPage() iterates it under
  // program_binary_lock_ - take the same lock, then nobody can be changing it underneath us
  ScopeLock lock(program_binary_lock_);
  size_t count = 0;
  for (Elf::Phdr& phdr : phdrs_)
  {
    if (count == max)
      break;
    out[count].vaddr = phdr.p_vaddr;
    out[count].memsz = phdr.p_memsz;
    out[count].filesz = phdr.p_filesz;
    out[count].flags = phdr.p_flags;
    ++count;
  }
  return count;
}
```

The syscall fills a kernel array first and copies it to user space in one `memcpy`.
