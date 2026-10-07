# A2-10 · RAM obfuscation thread

**Time box 60 min · ★★★ · official task 28 · read first: ex06**

```
Name:         RAM Obfuscation Thread
Nr:           28
Description:  Write a kernel thread that occasionally relocates the contents of PPNs and
              their mappings to a random other available PPN.
Parameters:   -
Return Value: -
Notes:        Take special care with pagefaults occurring during the remap process.
              (Syscall 1780 relocations() for the test: pages moved in this process.)
```

## Test

`task.sh start a2-10` → `task.sh test` (2 runs). Done when pages of the test process were moved
and none of its data (16 pages + a deep stack) was lost or corrupted.

## Hints

<details><summary>1 – finding the processes</summary>

The thread needs every address space. Keep a list of all Loaders (an intrusive list: a static
head pointer needs no constructor) under a global Mutex created in `startup()`. Loaders register in
the constructor and unregister **first thing** in the destructor.
</details>

<details><summary>2 – lifetime</summary>

Hold the list lock while working on a loader: its destructor blocks on the same lock to unregister,
so it cannot die under you. Lock order: list lock → that loader's page-table lock. Never reverse.
</details>

<details><summary>3 – one relocation</summary>

Under the page-table lock: pick a present page, `present = 0` + TLB flush, copy old → new (ident
mapping), point the entry to the new page, present again, free the old one.
</details>

<details><summary>4 – 'page faults during the remap'</summary>

If the thread is preempted mid-copy, a thread of that process may touch the page: it faults (not
present). The page fault handler must **not** treat it as a new page – a stack page would end up in
`loadPage` → "no section" → killed. Check under the page-table lock (which waits for the relocation)
whether the page is mapped again; if so, just return and repeat the access.
</details>

## Questions a tutor might ask

- Why present = 0 before copying, not after?
- What exactly happens to a thread that faults on the page during the copy?
- Why is the TLB of the target process not a problem on one CPU – and on several?
- Which deadlock would the reverse lock order cause?
