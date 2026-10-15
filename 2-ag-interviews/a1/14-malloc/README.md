# A1-14 · malloc / free / calloc / realloc

**Time box 30 min · ★★☆ · type U (user space only) – see [TRIAGE.md](../../TRIAGE.md)**

```
Name:         malloc, free, calloc, realloc (stdlib.h - the stubs exist)
Description:  A heap for user programs, entirely in the libc - no syscall.
              malloc(size) returns a 16-byte aligned block of at least size bytes, or
              NULL (size 0, or not enough memory). free(ptr) gives a block back;
              a later malloc may hand the same memory out again. free(NULL) does nothing.
              calloc(n, size): n * size bytes, all zero. realloc(ptr, size): a block of
              size bytes with the old content (realloc(NULL, s) = malloc(s)).
              The heap may be a fixed pool of 1 MiB.
Return Value: the block, or NULL
```

## Test

`task start a1-14` → `task test` (runs `malloc_basic.sweb`, then `exit` for the leak check).
Done when 9 checks pass.

## Your team's code

The stubs are in `userspace/libc/src/stdlib.c` (all return 0 / do nothing), declared in
`userspace/libc/include/stdlib.h`. `memset`/`memcpy` are in `string.h`. Nothing in the kernel
changes – no syscall.

## Hints

<details><summary>1 – where does the memory come from?</summary>

A big `static char pool[1024 * 1024];` in `stdlib.c`. It is in `.bss`: it costs nothing until
a page is touched – then the loader maps a zero page (lazy loading). Every process has its
own copy, because every process has its own address space.
</details>

<details><summary>2 – how does free() know the size?</summary>

Put a small header **before** every block: `struct BlockHeader { size_t size; size_t used; }`.
malloc returns `header + 1` (the bytes right after it); free does `(struct BlockHeader*) ptr - 1`
to get back to the header. 16 bytes of header keep the blocks 16-byte aligned.
</details>

<details><summary>3 – finding a block</summary>

Walk the blocks from the start of the pool: `offset += sizeof(header) + size`. The first block
that is free and big enough is used (first fit). None found → append a new block at the end
of the used part (`pool_end`). Round the size up to 16 first.
</details>

<details><summary>4 – calloc and realloc</summary>

calloc: check that `n * size` does not overflow, malloc, memset 0 – a reused block contains
old data. realloc: if the block is already big enough, return it; else malloc a new one,
memcpy the old size, free the old one.
</details>

## Questions a tutor might ask

- Why is malloc not a syscall? When would it need one? (a pool that can grow: `sbrk`, A1-12, or `mmap`)
- What is fragmentation? Show it with your implementation. How would merging free neighbours help?
- Two threads call malloc at the same time – what can go wrong? How would you fix it? (a lock, A1-15)
- What happens on `free` of a pointer malloc never returned, or a double free?
