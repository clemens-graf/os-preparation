# A1-10 · a page shared by two processes

**Time box 45 min · ★★★**

**Theory first:** [concepts.pdf](../../concepts.pdf) §16 frames, shared pages, §4 page faults

```
Name:         shm_attach, shm_detach
Nr:           1680, 1681
Description:  shm_attach(key) maps the shared page of key (1..15) into the calling
              process (at a fixed address per key) and returns its address. The first
              attach of a key creates the page (zeroed). Attaching twice in one process
              returns the same address.
              shm_detach(key) unmaps it. The page lives as long as at least one process
              has it attached - also if a process exits without detaching.
Return Value: attach: address or NULL; detach: 0 or -1 (not attached)
```

## Test

`task.sh start a1-10` → `task.sh test`. `shm_basic.sweb` starts `shm_child.sweb` twice (the
second time it exits without detaching). Done when 9 checks pass and the final `exit` reports no
leak.

## Your team's code

`SharedPages` (new, `common/{include,source}/mm/`, created in `startup()`), `shm_attached_` bit mask + `shmAttach/shmDetach` in `UserProcess` (under `thread_lock_`), cleanup in `~UserProcess` **before** `delete arch_memory_` with `unmapPage(vpn, false)` – a new parameter of `ArchMemory::unmapPage`: do not free the shared frame.

## Hints

<details><summary>1 – one page, two owners</summary>

`unmapPage` frees the physical page – fatal for a page another process still uses, and
`~ArchMemory` frees every present page at exit. You need a reference count per key and an
unmap that does not free.
</details>

<details><summary>2 – a kernel-wide table</summary>

Global objects are never constructed in SWEB: create the table (and its Mutex) with `new` in
`startup()`.
</details>

<details><summary>3 – lock order</summary>

Two locks: the global table lock and the process' page-table lock. Never hold both: take a
reference first (table lock), then map (page-table lock). Detach: unmap first, then drop the
reference. At exit: the Loader's destructor unmaps the attached keys and drops their references.
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>What happens without the reference count when the parent exits first?</summary>

The parent's exit runs `~ArchMemory`, which frees every present page – including the shared frame. The child still maps it and now reads/writes a frame that the PageManager may hand to someone else (use after free). At the child's exit it is freed a second time → "Double free PPN".

</details>

<details><summary>Why take the reference BEFORE mapping?</summary>

If you mapped first and took the reference afterwards, there'd be a moment where the frame is mapped but its count doesn't include you: another process detaching (or exiting) in that moment sees count 0 and frees the frame while you're about to use it. Reference first = the frame can't disappear under you.

</details>

<details><summary>Where does process exit clean up, and in which thread does that run?</summary>

In the process's destructor: `~UserProcess` (before `delete arch_memory_`) detaches every shared page – unmap **without** freeing, then drop the reference (free only if it was the last). That destructor runs in the **CleanupThread**, which deletes the last dead thread → so it must not rely on the process's `cr3` being active.

</details>

<details><summary>How would you give it names instead of numbers (shm_open)?</summary>

A table `name → key` in the kernel (`shm_open(name)`): copy the name in (`copyUserString`), look it up under a global lock, create an entry if it doesn't exist, return a handle; `shm_unlink(name)` removes the name (the page stays until the last detach). The numbers then become internal.

</details>
