# A2-02 · createPageBackup / loadPageBackup

**Time box 40 min · ★★☆ · type S ([TRIAGE](../../TRIAGE.md)) · builds on A2-01 · official task 31**

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

## Your team's code

Builds on A2-01. `backups_[32]` + `createPageBackup/loadPageBackup` in `UserProcess` (under `thread_lock_`); the slots are freed in `~UserProcess`.

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

Answer each one out loud first – then open it to compare.

<details><summary>Why keep the page-table lock while writing the backup?</summary>

So the page can't change, be swapped out or be unmapped **during** the disk write – otherwise the backup could be torn, or written from a frame that was already freed and reused. Holding a Mutex during disk I/O is fine (the request sleeps).

</details>

<details><summary>What happens to the backup if the page is swapped out in between?</summary>

The backup slot and the swap slot are different slots: swapping the page out writes it into a **new** swap slot, the backup slot stays untouched. `loadPageBackup` on a swapped page must swap it in first (or write into the swap slot) – the solution requires the page to be present, i.e. it faults it back in first.

</details>

<details><summary>Who frees the backup slots?</summary>

`~UserProcess` – it frees all slots in `backups_` (otherwise `swapinfo` never returns to 0 and the device fills up). An overwrite of an existing backup reuses its slot, so slots don't pile up while the process runs.

</details>
