# A1-14 · malloc / free / calloc / realloc

**Time box 30 min · ★★☆ · type U (user space only) – see [TRIAGE.md](../../TRIAGE.md)**

**Theory first:** [concepts.pdf](../../concepts.pdf) §2 heap & malloc, §1 address space · example: [ex14](../../examples/ex14-libc-function/)

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

Answer each one out loud first – then open it to compare.

<details><summary>Why is malloc not a syscall? When would it need one? (a pool that can grow: <code>sbrk</code>, A1-12, or <code>mmap</code>)</summary>

Your `malloc` only hands out pieces of memory the process **already has** – a static pool in `.bss`. Nothing only the kernel can do. It needs the kernel as soon as the pool must **grow**: then it asks for more address space with `sbrk` (move the program break, A1-12) or `mmap` (A2-13) – real mallocs do exactly that.

</details>

<details><summary>What is fragmentation? Show it with your implementation. How would merging free neighbours help?</summary>

Free memory split into pieces that are each too small. Example: malloc 1 KiB (A), 1 KiB (B), 1 KiB (C); free A and C – 2 KiB are free, but `malloc(2048)` can't use them (two separate 1 KiB holes) and appends at the end. Your version also never **splits** a big free block (a 16-byte request takes a whole free 1 KiB block). Merging free neighbours (coalescing) and splitting would let freed space be reused.

</details>

<details><summary>Two threads call malloc at the same time – what can go wrong? How would you fix it? (a lock, A1-15)</summary>

Both walk the block list at the same time, both find the **same** free block, both set `used = 1` and return the same pointer – two owners of one block. Or both append and compute the same `pool_end`. Fix: a lock around the whole `malloc`/`free` (the A1-15 mutex) – the search and the `used = 1` must be one critical section.

</details>

<details><summary>What happens on <code>free</code> of a pointer malloc never returned, or a double free?</summary>

`free(p)` goes to `(BlockHeader*) p - 1` and sets `used = 0` – for a pointer malloc never returned, that "header" is random data, and writing it corrupts memory (or the pool's structure). A double free marks a block free twice – harmless in your version until the block is handed out again and two parts of the program use it. Real mallocs detect some of this (magic numbers, checks) and abort.

</details>
