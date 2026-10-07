# A2-08 · NX: never execute data or stack

**Time box 25 min · ★★☆ · read first: ex09**

```
Name:         if you try to execute a page that is not in the code segment, it causes a
              page fault (and the process is killed)
Description:  The loader sets the NX bit (execution_disabled) on every page that contains
              no executable segment (PF_X), and the stack page is NX too.
              The page fault handler reports such a fault clearly.
```

## Test

`task.sh start a2-08` → `task.sh test`. Done when calling into `.data` and into the stack kills
the program, and `mult.sweb` still works.

## Hints

<details><summary>1 – where</summary>

`Loader::loadPage` (like A0-05, with PF_X instead of PF_W) and `UserProcess`'s constructor for the stack page.
</details>

<details><summary>2 – the fault</summary>

Instruction fetch from a present NX page: `present = 1`, `fetch = 1` → `checkPageFaultIsValid` rejects it.
</details>

## Questions a tutor might ask

- What is the attack this prevents?
- How would a JIT compiler work under such a policy?
