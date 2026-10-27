# A2-08 · NX: never execute data or stack

**Time box 25 min · ★★☆ · type E (loader) ([TRIAGE](../../TRIAGE.md)) · read first: ex09**

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

## Your team's code

Your team's `Loader::loadPage(address, arch_memory)` runs **without** the page-table lock: set NX right after `mapPage` (or give `mapPage` an NX parameter, like A0-05's `writeable`). Stack pages: `UserProcess::mapStackPage` (every stack page goes through it – also the ones the page fault handler adds).

## Hints

<details><summary>1 – where</summary>

`Loader::loadPage` (like A0-05, with PF_X instead of PF_W) and `UserProcess::mapStackPage` for every stack page.
</details>

<details><summary>2 – the fault</summary>

Instruction fetch from a present NX page: `present = 1`, `fetch = 1` → `checkPageFaultIsValid` rejects it.
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>What is the attack this prevents?</summary>

Running injected code: a buffer overflow writes machine code onto the stack/heap and overwrites a return address to jump into it. With NX on every non-code page the jump faults (instruction fetch from an NX page) instead of running the attacker's code.

</details>

<details><summary>How would a JIT compiler work under such a policy?</summary>

It generates code into a **writable, non-executable** page, then switches that page to **executable, non-writable** (`mprotect`) before running it – never both at once. Or two mappings of the same frame: one writable, one executable (at different addresses).

</details>
