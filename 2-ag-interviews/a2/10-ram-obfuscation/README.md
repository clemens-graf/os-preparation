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

## Your team's code

The list of address spaces is a list of **processes**: `static UserProcess* all_processes_`, `all_processes_lock_` (created in `startup()`), `next_process_`; the constructor registers (also for `fork`), `~UserProcess` unregisters first. `relocatePage` / `isMapped` in `UserProcess` under `thread_lock_`; `isMapped` goes first in the page fault handler's valid branch.

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

Answer each one out loud first – then open it to compare.

<details><summary>Why present = 0 before copying, not after?</summary>

While the copy runs, a thread of the process may run (the kernel thread can be preempted) and **write** to the old frame – that write would be lost, because the mapping then switches to the copy taken before. With `present = 0` + flush first, such a write faults and waits; after the remap it is repeated on the new frame.

</details>

<details><summary>What exactly happens to a thread that faults on the page during the copy?</summary>

It page-faults (not present). The handler first asks `isMapped(vpn)`: that takes the process's `thread_lock_` – which the relocation holds – so it **waits** until the copy is done, then sees the page present and returns; the instruction is repeated and works. Without that check it would call `loadPage` (data loss, or exit 666 for a stack page).

</details>

<details><summary>Why is the TLB of the target process not a problem on one CPU – and on several?</summary>

On one CPU the target process isn't running while the kernel thread runs, and when it runs again its `cr3` is loaded – which flushes the TLB. On several CPUs a thread of the process may be running **right now** on another CPU with the old translation in its TLB: you'd need a **TLB shootdown** (an inter-processor interrupt that makes the other CPUs flush).

</details>

<details><summary>Which deadlock would the reverse lock order cause?</summary>

The thread takes `all_processes_lock_` → one process's `thread_lock_`. If some path took a process's `thread_lock_` and then `all_processes_lock_` (e.g. a destructor unregistering while holding `thread_lock_`), the two could each hold one lock and wait for the other forever. One fixed order everywhere prevents the cycle.

</details>
