# A1-08 snapshot & revive – solution notes

Patch: [`solution.patch`](solution.patch).

## Data

```cpp
struct ThreadSnapshot
{
  size_t rip, rflags, rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, r8, ..., r15;
  size_t stack_size;
  char stack[SNAPSHOT_MAX_STACK];
};
// Thread: ThreadSnapshot* snapshot_ (deleted in ~Thread)
```

## Why the stack

```
pthread_snapshot()  -> __syscall frame at address X  (snapshot saves rip/rsp pointing into it)
... returns, more code runs, deeper calls overwrite X ...
pthread_revive()    -> __syscall frame at address X again (same depth!)
```

Restoring only the registers jumps into a frame whose saved `rbp` and return address now belong to
`pthread_revive`'s call: it would "return" from revive, not from snapshot. Saving
`[rsp, stack top)` and writing it back restores every frame exactly. That is the difference to
`setjmp`: setjmp saves the state at its *caller's* level and requires that caller to still be alive.

## Registers: field by field

`ArchThreadRegisters` contains `uint8* fpu` and a destructor that `delete[]`s it. Copying the whole
struct into the snapshot would share that buffer – the destructor of the copy frees the thread's FPU
area. (The FPU state is not saved – floating point values in registers are not restored.)

## Return value

`revive()` restores all registers and returns 1: `syscallHandler` writes it into `rax` –
the restored `rip` is the instruction after `int $0x80` of the old snapshot call.

## Limits / variants

- One snapshot per thread; stack ≤ 16 KiB; with threads, the stack top is the thread's own.
- Variant: snapshot into a user buffer (`jmp_buf`-like) – then check the pointer, copy out/in.
