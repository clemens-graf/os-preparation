# A1-13 · a syscall with 15 arguments

**Time box 15 min · ★☆☆**

**Theory first:** [concepts.pdf](../../concepts.pdf) §8 registers & calling convention

```
Name:         sum15
Nr:           1710
Description:  sum15(a1, ..., a15) returns 1*a1 + 2*a2 + ... + 15*a15, computed in the
              kernel. __syscall can only carry 5 values: fetch the remaining arguments
              from user memory.
Return Value: the sum, -1 on a bad pointer
```

## Test

`task.sh start a1-13` → `task.sh test`. Done when 4 checks pass (the weights make a wrong order visible).

## Your team's code

Only the six places – no team-specific state. Number 1710 is free in your block.

## Hints

<details><summary>1 – how to get 15 values across</summary>

The libc wrapper keeps 4 in registers and puts the other 11 into an array on its stack; the
5th syscall argument is the array's address. The kernel checks the range and copies it in.
</details>

<details><summary>2 – the other way: from the caller's stack</summary>

In the System V ABI arguments 7..15 *are* on the stack already (above the return address of
`sum15`). Reading them from the kernel via the frame pointers is possible but fragile – the
array is the robust way.
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Where are arguments 7–15 of a normal C call? Arguments 1–6?</summary>

Arguments 1–6 in `rdi rsi rdx rcx r8 r9` (System V ABI). Arguments 7–15 on the **stack**, pushed by the caller right above the return address (argument 7 at `rsp + 8` when the function starts). SWEB's syscalls are different: number in `rax`, arguments in `rbx rcx rdx rsi rdi` – only five.

</details>

<details><summary>Why copy the array into the kernel instead of reading it in the loop?</summary>

Reading user memory in the loop means: every access can page-fault, and another thread of the process can change values between two reads (TOCTOU) – the sum would mix old and new values. One `memcpy` into a kernel array after checking the pointer, then compute only on the copy.

</details>
