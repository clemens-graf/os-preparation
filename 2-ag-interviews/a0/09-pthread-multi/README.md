# A0-09 · pthread_multi

**Time box 40 min (with 08 done) · ★★★ · builds on [A0-08](../08-pthread-create/)**

```
Name:         pthread_multi
Nr:           1552 (sc_pthread_multi)
Description:  Creates ONE new thread that calls functions[0](arguments[0]),
              functions[1](arguments[1]), ..., functions[count-1](arguments[count-1])
              one after the other, then ends.
              The caller may change or reuse both arrays immediately after the call
              returns.
Parameters:   pthread_t* thread, void* (**functions)(void*), void** arguments, size_t count
Return Value: 0, or -1 (count 0 or > 16, bad pointers, a function pointer that is not a
              user address)
Notes:        Start from your pthread_create (A0-08).
```

## Test

`task start a0-09` – this branch starts from the **A0-08 reference solution** (`base/a0-08`), so you
only add `pthread_multi`. If you prefer to build on your own A0-08, merge or cherry-pick your
`task/a0-08-pthread-create` branch into it first. Then `task test` (also re-runs `pthread_create_basic.sweb`).
Done when all 7 checks pass.

## Hints

<details><summary>1 – who runs the loop?</summary>

The loop "call each function" can live in libc: a start function
`multiStart(functions, arguments, count)` that loops and then calls `pthread_exit`. The
kernel only has to start the thread there with three arguments (`rdi`, `rsi`, `rdx`).
</details>

<details><summary>2 – "may reuse the arrays immediately"</summary>

The new thread reads the arrays much later than the syscall returns. So the kernel copies
them at the time of the call – into kernel memory first (after checking every pointer), then
onto the **new thread's stack**, and passes the addresses of these copies.
</details>

<details><summary>3 – writing to another thread's stack</summary>

Before the new thread is added to the scheduler, nobody else can touch it – you may set up
its stack and registers freely. Write through the ident mapping (`checkAddressValid`
returns the kernel address of a user address in that address space). Keep the stack
16-byte aligned and leave 8 bytes for the "return address".
</details>

## Questions a tutor might ask

- Why copy the arrays, why not just pass the user pointers on?
- Why copy into kernel memory first, and only then onto the new stack?
- Could you do the whole thing in user space with `pthread_create`? What would be the difference?
- What happens if one of the functions calls `pthread_exit`?
