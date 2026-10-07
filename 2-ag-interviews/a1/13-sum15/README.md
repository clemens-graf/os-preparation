# A1-13 · a syscall with 15 arguments

**Time box 15 min · ★☆☆**

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

- Where are arguments 7–15 of a normal C call? Arguments 1–6?
- Why copy the array into the kernel instead of reading it in the loop?
