# ex09 – the loader: segments of the running program

`segments(out, max)` copies the loadable ELF segments (program headers) of the calling
process to user space. Test: `ex_loader.sweb` (3 checks; prints the segment list).

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

```cpp
size_t Loader::getSegments(SegmentInfo* out, size_t max)
{
  ScopeLock lock(program_binary_lock_);     // the lock loadPage holds while it reads phdrs_
  ...copy p_vaddr, p_memsz, p_filesz, p_flags...
}
```

The syscall fills a kernel array first and copies it to user space in one `memcpy`.
