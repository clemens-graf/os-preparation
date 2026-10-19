# A1-14 malloc / free / calloc / realloc – solution notes

Patch: [`solution.patch`](solution.patch) – only `userspace/libc/src/stdlib.c`. Type U.

```c
#define POOL_SIZE (1024 * 1024)
struct BlockHeader { size_t size; size_t used; };          // 16 bytes: blocks stay 16-aligned
static char pool[POOL_SIZE] __attribute__((aligned(16)));  // .bss: costs nothing until touched
static size_t pool_end = 0;                                 // used part of the pool

void* malloc(size_t size)
{
  if (size == 0 || size > POOL_SIZE) return 0;
  size = (size + 15) & ~(size_t) 15;
  for (size_t offset = 0; offset < pool_end;)               // 1. first fit among the blocks
  {
    struct BlockHeader* block = (struct BlockHeader*) (pool + offset);
    if (!block->used && block->size >= size)
    {
      block->used = 1;
      return (char*) block + sizeof(struct BlockHeader);
    }
    offset += sizeof(struct BlockHeader) + block->size;
  }
  if (pool_end + sizeof(struct BlockHeader) + size > POOL_SIZE) return 0;   // 2. or append
  struct BlockHeader* block = (struct BlockHeader*) (pool + pool_end);
  block->size = size; block->used = 1;
  pool_end += sizeof(struct BlockHeader) + size;
  return (char*) block + sizeof(struct BlockHeader);
}

void free(void* ptr) { if (ptr) ((struct BlockHeader*) ptr - 1)->used = 0; }
```

- `(char*) block + sizeof(struct BlockHeader)` = the first byte **after** the header (as `char*`, so the
  addition counts in bytes). Short form you may see elsewhere: `block + 1` (counts in whole headers);
  `(struct BlockHeader*) ptr - 1` goes back to it.
- `calloc`: overflow check `nmemb > POOL_SIZE / size`, then malloc + `memset 0` – a reused block
  contains old data (the test writes `0x55` into it first).
- `realloc`: big enough → same pointer; else malloc, `memcpy(old size)`, free.
- The pool is in `.bss`: the loader maps its pages lazily (zero pages) – 1 MiB costs nothing
  until it is used.

Tutor topics: fragmentation (a freed 1 KiB block is never split or merged here – merging free
neighbours would help), thread safety (none – a lock from A1-15 around malloc/free), and when a
real malloc needs the kernel (a pool that grows: `sbrk` A1-12, or `mmap`).
