# A1-10 · a page shared by two processes

**Time box 45 min · ★★★ · type S + E (exit) ([TRIAGE](../../TRIAGE.md))**

**Theory first:** [concepts.pdf](../../concepts.pdf) §16 frames, shared pages, §4 page faults

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

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

## Level 2 – hints

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

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. A global table (key → frame + reference count) in a new class with its own lock, created in `startup()` (no global constructors).
2. `ArchMemory::unmapPage` gets a parameter "free the frame or not" (default: free).
3. Per process: a bit mask of attached keys (under `thread_lock_`).
4. `shm_attach(key)`: check the key (1–15); first take a **reference** in the global table (creates a zeroed frame if new); then under `thread_lock_`: not attached yet → map it at the key's fixed address and set the bit; if it was attached already → drop the extra reference. Return the address.
5. `shm_detach(key)`: under `thread_lock_` unmap **without freeing** + TLB flush + clear the bit; then drop the reference (the table frees the frame at 0).
6. In `~UserProcess` (before the address space is deleted): do the detach for every set bit.
7. Never hold both locks at once. `task test`.

</details>

## Your team's code

`SharedPages` (new, `common/{include,source}/mm/`, created in `startup()`), `shm_attached_` bit mask + `shmAttach/shmDetach` in `UserProcess` (under `thread_lock_`), cleanup in `~UserProcess` **before** `delete arch_memory_` with `unmapPage(vpn, false)` – a new parameter of `ArchMemory::unmapPage`: do not free the shared frame.

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
