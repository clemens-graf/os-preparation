# A2-02 · createPageBackup / loadPageBackup

**Time box 40 min · ★★☆ · builds on A2-01 · official task 31**

```
Name:         Syscall createPageBackup() & loadPageBackup()
Nr:           1730, 1731
Description:  createPageBackup takes a virtual address and stores the current content of
              the associated page on the disk, but the page is not actually swapped out
              (present stays 1). If the page already has a backup on the disk, overwrite
              it and do not use additional disk space. loadPageBackup on the same address
              loads the backup from the disk and sets the page to the backup content.
              Create a new type of request for this task.
              Example:  int a = 5;  printf("%d, ", a); createPageBackup(&a); a = 6;
                        printf("%d, ", a); loadPageBackup(&a); printf("%d\n", a);
              Output:   5, 6, 5
Parameters:   a virtual address
Return Value: 0 if a backup is possible / was loaded, otherwise -1
Notes:        Pick an address that is not on the stack (the stack holds essential registers).
```

## Test

`task.sh start a2-02` → `task.sh test`. Done when 7 checks pass (the example prints 5, 6, 5) and
the backup's slot is freed when the process exits.

## Hints

<details><summary>1 – 'a new type of request'</summary>

In your team's design that means a new request type for your swap thread/queue. Here: two new
`SwapManager` operations next to `writeOut`/`readIn`: `allocSlot()` and `writeSlot(slot, ppn)`
(write without unmapping).
</details>

<details><summary>2 – where to remember the backup</summary>

Per process, a small table vpn → slot (under the page-table lock). Second backup of the same vpn:
same slot, overwrite.
</details>

<details><summary>3 – load in place</summary>

Read the slot straight into the page's physical page (`readIn(slot, ppn)` writes through the ident
address). The page stays mapped; the process sees the old content immediately.
</details>

## Questions a tutor might ask

- Why keep the page-table lock while writing the backup?
- What happens to the backup if the page is swapped out in between?
- Who frees the backup slots?
