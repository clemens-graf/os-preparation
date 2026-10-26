# A1-13 · a syscall with 15 arguments

**Time box 15 min · ★☆☆**

**Theory first:** [concepts.pdf](../../concepts.pdf) §8 registers & calling convention

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

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

## Level 2 – hints

<details><summary>1 – how to get 15 values across</summary>

The libc wrapper keeps 4 in registers and puts the other 11 into an array on its stack; the
5th syscall argument is the array's address. The kernel checks the range and copies it in.
</details>

<details><summary>2 – the other way: from the caller's stack</summary>

In the System V ABI arguments 7..15 *are* on the stack already (above the return address of
`sum15`). Reading them from the kernel via the frame pointers is possible but fragile – the
array is the robust way.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. libc: write `sum15(a1, …, a15)`: put arguments 5 to 15 into a local array and pass **four** arguments plus the array's address to `__syscall` (only five values fit).
2. Kernel: syscall plumbing (1710) with five parameters: four numbers and the array address.
3. Check the array address: not 0, and the whole array (11 values) lies below `USER_BREAK`.
4. Copy the 11 values **once** into a kernel array (one copy, then only use the copy).
5. Compute 1×a1 + 2×a2 + … + 15×a15 and return it.
6. `task test`.

</details>

## Your team's code

Only the six places – no team-specific state. Number 1710 is free in your block.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Where are arguments 7–15 of a normal C call? Arguments 1–6?</summary>

Arguments 1–6 in `rdi rsi rdx rcx r8 r9` (System V ABI). Arguments 7–15 on the **stack**, pushed by the caller right above the return address (argument 7 at `rsp + 8` when the function starts). SWEB's syscalls are different: number in `rax`, arguments in `rbx rcx rdx rsi rdi` – only five.

</details>

<details><summary>Why copy the array into the kernel instead of reading it in the loop?</summary>

Reading user memory in the loop means: every access can page-fault, and another thread of the process can change values between two reads (TOCTOU) – the sum would mix old and new values. One `memcpy` into a kernel array after checking the pointer, then compute only on the copy.

</details>
