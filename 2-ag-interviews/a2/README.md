# A2 – interview tasks

The A2 interview is about **virtual memory**: swapping, copy-on-write, page-table bits, the
swap device – and the A1 tasks may come again. Base SWEB has no swapping, so A2-01 builds a
*minimal* one (swap single pages out to the swap partition `idea2` and back in on the page
fault); A2-02 to A2-06 build on it. In the interview you work in your team's A2 implementation
(a swap thread, a page replacement policy, an inverted page table …) – the mechanisms are the
same.

## The tasks

| # | Task | From the list | Builds on | Time box | Level |
|---|---|---|---|---|---|
| 01 | [swap a page out and back in](01-swap/) | (the core of A2) | – | 60 min | ★★★ |
| 02 | [createPageBackup / loadPageBackup](02-page-backup/) | official task 31 | 01 | 40 min | ★★☆ |
| 03 | [swap onto the kernel heap](03-heap-swap/) | *swap-out onto kernel-heap* | 01 | 25 min | ★★☆ |
| 04 | [XOR-encrypted swap](04-xor-swap/) | 7) encrypted writing-out | 01 | 25 min | ★★☆ |
| 05 | [don't write unchanged pages](05-dirty-swap/) | 5) not writing unchanged pages | 01 | 40 min | ★★★ |
| 06 | [F3: swap out all pages of a process](06-f3-swap-all/) | 16)/17) swap out all pages, F3 | 01 | 30 min | ★★☆ |
| 07 | [mprotect + W⊕X](07-mprotect/) | 14) mprotect, 15) W xor X | – | 30 min | ★★☆ |
| 08 | [NX: never execute data or stack](08-nx/) | 19) stack executable, 23) exec outside code | – | 25 min | ★★☆ |
| 09 | [zero-page deduplication + copy-on-write](09-zero-page/) | 1) deduplication zero | – | 45 min | ★★★ |
| 10 | [RAM obfuscation thread](10-ram-obfuscation/) | official task 28 | – | 60 min | ★★★ |
| 11 | [boot counter / message of the day](11-boot-counter/) | 12)/13) boot counter, motd | – | 40 min | ★★☆ |
| 12 | [syscall blacklist](12-syscall-blacklist/) | 11) blacklist syscall | – | 20 min | ★☆☆ |

Not here (sketches in `../guide.pdf`, chapter A2): disk defragmentation (needs your swap bitmap),
deduplication of arbitrary pages (hash + COW like 09), mmap of a file (09's page fault + the
loader's `readFromBinary`), ASLR (randomise `USER_BREAK - x` for the stack in `UserProcess`),
checksum check (05's "clean copy" + a checksum in the PTE bits or a table).

## How to work on a task

```bash
task.sh start a2-01        # branch task/a2-01-swap
task.sh test               # build + run the tests headless (with leak check)
task.sh reset a2-01        # start over (backup kept)
task.sh solution a2-01     # the reference solution on its own branch
```

Syscall numbers 1720–1799.

## The A2 locking questions

1. A page is being written to disk – what stops another thread from writing to it meanwhile?
2. A thread faults on a page that is being swapped in by another thread – what happens?
3. Which locks are held during disk I/O? (Never a SpinLock – the request sleeps.)
4. Lock order between the page-table lock, the swap bookkeeping and the PageManager?
5. When may a physical page be freed – and who else could still have it mapped (shared, COW)?
