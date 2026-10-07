# A1-10 · a page shared by two processes

**Time box 45 min · ★★★**

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

- What happens without the reference count when the parent exits first?
- Why take the reference BEFORE mapping?
- Where does process exit clean up, and in which thread does that run?
- How would you give it names instead of numbers (shm_open)?
