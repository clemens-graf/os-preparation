# A2-06 · F3: swap out all pages of a process

**Time box 30 min · ★★☆ · builds on A2-01 · read first: ex07**

```
Name:         f3 - swap out/in all pages of a process
Description:  When F3 is pressed, all present pages of a process are swapped out: the next
              process that makes a syscall is the one. (Its pages come back on demand.)
              "Set a flag when F3 is pressed and catch it in the syscall handler at the top."
```

## Test

`task.sh start a2-06` → `task.sh test`: starts `fkey_swap_check.sweb`, presses F3 while it runs,
then checks that its pages were swapped out and come back intact.

## Your team's code

Builds on A2-01. `Console::handleKey` sets `Syscall::f3_pending_`; the top of `Syscall::syscallException` calls `getProcess()->swapOutAll()` – your team's cancellation check is there too.

## Hints

<details><summary>1 – the context problem</summary>

`Console::handleKey` runs in the Console kernel thread, not in the process: wrong page tables,
racing with the process. Flag + work in the next syscall (ex07).
</details>

<details><summary>2 – walking while changing</summary>

`swapOut` takes the page-table lock and changes the tables. Collect the vpns first (under the
lock), release, then swap them out one by one (each may fail if the page changed meanwhile).
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why not swap the pages out directly in handleKey?</summary>

`handleKey` runs in the **Console kernel thread**: the wrong page tables are active (the kernel's, not the process's), and the process may be running or faulting at the same moment without any coordination. Setting a flag and letting the process do it at its next syscall means: the right address space, a normal thread context, with interrupts on, able to take `thread_lock_`.

</details>

<details><summary>Which process gets 'hit' – and how would you target a specific one?</summary>

The process of whichever thread makes the **next syscall** – usually the one that is running/printing. To hit a specific one: store a target pid with the flag, and only that process's syscall entry acts on it (others ignore it) – or a kernel thread iterates all processes and swaps out under each process's lock.

</details>

<details><summary>Could the swapped-out pages include the code that is running right now? Why is that fine?</summary>

Yes – the code and stack pages in use are swapped out too. The very next instruction page-faults and swaps them back in: correct, just slow. It works because the fault handler itself is kernel code that is never swapped, and the swap-out happens under the lock with the page not present – no instruction runs on a half-swapped page.

</details>
