# A1-13 15-argument syscall – solution notes

Patch: [`solution.patch`](solution.patch).

```c
size_t sum15(size_t a1, ..., size_t a15)
{
  size_t rest[11] = {a5, a6, ..., a15};
  return __syscall(sc_sum15, a1, a2, a3, a4, (size_t) rest);
}
```

```cpp
size_t Syscall::sum15(size_t a1, size_t a2, size_t a3, size_t a4, size_t rest)
{
  if (rest == 0 || rest >= USER_BREAK - 11 * sizeof(size_t)) return (size_t) -1;
  size_t args[15] = {a1, a2, a3, a4};
  memcpy(&args[4], (void*) rest, 11 * sizeof(size_t));   // copy in once
  ...
}
```

## Background

System V AMD64: arguments 1–6 in `rdi rsi rdx rcx r8 r9`, 7+ on the stack above the return
address. SWEB's syscall ABI: number in `rax`, 5 arguments in `rbx rcx rdx rsi rdi`. "Fetch the
remaining arguments from the stack" literally: the kernel could follow `user_registers_->rbp`
(the `__syscall` frame) to the frame of `sum15` and read its stack arguments – it works with
`-O0` frame pointers, but breaks as soon as the compiler changes the layout. The array is the
robust version; mention both.

## Shortest working version (tested)

[`short.patch`](short.patch) – **19 lines** instead of 33, passes the same test. Get it with
`task short a1-13` (branch `short/…`, then `task test`).

What is shorter: inline in the `case`: reads the 11 arguments straight from the program's array, no pointer check, no `memcpy`.

Use it to see the **core** of the task. In the interview, the full solution's extra checks are what
a tutor asks about ("what if the pointer is invalid / two threads at once?").
