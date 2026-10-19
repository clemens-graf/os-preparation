# A1-08 · snapshot & revive

**Time box 40 min · ★★★**

**Theory first:** [concepts.pdf](../../concepts.pdf) §9 diverting a thread, §8 registers & calling convention

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

## Your team's code

Per thread → `UserThread.h`: `ThreadSnapshot* snapshot_ = nullptr;` (deleted in `~UserThread`). The stack top is **this thread's** stack slot: `((UserThread*) currentThread)->getUserStackTop()`, not `USER_BREAK`. `SNAPSHOT_MAX_STACK` = 4 pages = `USERSTACK_PAGES`.

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

Answer each one out loud first – then open it to compare.

<details><summary>Why does the snapshot need the stack? Give the exact sequence that breaks without it.</summary>

The saved `rsp` points into the stack: the return addresses and locals of the frames above it are only *there*. Sequence: `snapshot()` saves registers, returns into `main`; main calls `f()`, whose frame overwrites the stack area where `snapshot`'s caller frames were (or `main` itself returns); `f` calls `revive()` → registers are restored, but the stack now contains `f`'s data → `ret` jumps to a garbage return address. Copying the used stack area back restores the frames.

</details>

<details><summary>Which registers did you save? Why not the fpu?</summary>

All user registers of `user_registers_` (`ArchThreadRegisters`: general registers, `rip`, `rsp`, `rflags`, segments) – copied **field by field**, not as a whole struct: the struct holds a pointer `fpu` and its destructor `delete[]`s it, so a struct copy would share and later free the thread's FPU buffer. The FPU state itself isn't saved: the test programs keep no floating-point values across a snapshot.

</details>

<details><summary>What would break with more threads (stack top, other threads' stacks)?</summary>

The stack copy is between the saved `rsp` and **this thread's** stack top (its own slot) – with several threads each has its own top, so it must be `((UserThread*) currentThread)->getUserStackTop()`, not a constant. Revive restores only the snapshotting thread's stack; other threads' stacks and globals keep their current state – a snapshot of one thread, not of the process.

</details>

<details><summary>How does user-space setjmp get away with saving only a few registers?</summary>

`setjmp` is a **function call**: by the ABI the caller already assumes the caller-saved registers are destroyed, so `setjmp` only saves the callee-saved ones (`rbx rbp r12–r15`), `rsp` and the return address. And `longjmp` may only jump to a frame that is **still alive** (not returned) – so the stack doesn't need copying. The kernel snapshot can't make either assumption.

</details>
