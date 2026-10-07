# A1-08 · snapshot & revive

**Time box 40 min · ★★★**

```
Name:         pthread_snapshot, pthread_revive
Nr:           1660, 1661
Description:  pthread_snapshot() saves the complete state of the calling thread -
              registers AND the used part of its user stack - in the kernel and
              returns 0. pthread_revive() puts the thread back to exactly that point:
              pthread_snapshot() returns a second time, now with 1. Local variables have
              their values from the snapshot again; global variables are not restored.
              Like setjmp/longjmp, but in the kernel - and it also works when revive is
              called from deeper functions that overwrote the snapshot's stack frames.
Return Value: snapshot: 0 / 1 (revived) / -1 (stack too big); revive: -1 if no snapshot
```

## Test

`task.sh start a1-08` → `task.sh test`. Done when all 5 checks pass.

## Hints

<details><summary>1 – why registers alone are not enough</summary>

The saved `rip`/`rsp` point into the `__syscall` frame of the `pthread_snapshot()` call. By the
time `pthread_revive()` runs, that frame (and maybe its caller's) has been overwritten – by the
revive call itself, which lies at the same addresses. Save the stack from `rsp` up to the stack
top too, and write it back.
</details>

<details><summary>2 – how to copy the registers</summary>

Field by field. `ArchThreadRegisters` has an `fpu` pointer and a destructor that frees it: a
struct copy would share and later double-free the FPU buffer.
</details>

<details><summary>3 – the return value</summary>

Restore all registers, return 1 from the revive syscall: it goes into `rax` – and the restored
`rip` is right after `int $0x80` in the old snapshot call.
</details>

## Questions a tutor might ask

- Why does the snapshot need the stack? Give the exact sequence that breaks without it.
- Which registers did you save? Why not the fpu?
- What would break with more threads (stack top, other threads' stacks)?
- How does user-space setjmp get away with saving only a few registers?
