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

- Why not swap the pages out directly in handleKey?
- Which process gets 'hit' – and how would you target a specific one?
- Could the swapped-out pages include the code that is running right now? Why is that fine?
