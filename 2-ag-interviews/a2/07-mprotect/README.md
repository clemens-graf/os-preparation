# A2-07 · mprotect + W⊕X

**Time box 30 min · ★★☆ · read first: ex04**

```
Name:         mprotect (sys/mman.h - stub and PROT_* constants exist)
Nr:           1760
Description:  mprotect(addr, len, prot) changes the access rights of the pages in
              [addr, addr + len): PROT_NONE / PROT_READ / PROT_WRITE / PROT_EXEC.
              W xor X policy: a page may never be writable and executable at the same
              time - such a request fails. All or nothing: if one page in the range is not
              mapped, nothing changes.
Return Value: 0, or -1 (addr not page aligned, len 0, unmapped page, WRITE|EXEC)
```

## Test

`task.sh start a2-07` → `task.sh test`. Done when 7 checks pass and calling code on a page made
non-executable kills the process.

## Your team's code

`process->getAddressSpace()` under `process->getThreadLock()`; the PTE bits (`user_access`, `writeable`, `execution_disabled`) via `resolveMapping(vpn).pt[pti]`. A write to a protected page lands in your `checkPageFaultIsValid` (present) → exit 9999. The stub is in `userspace/libc/src/mman.c`.

## Hints

<details><summary>1 – the PTE bits</summary>

`writeable`, `execution_disabled` (NX - works, EFER.NXE is on), `user_access` (0 for PROT_NONE).
</details>

<details><summary>2 – lazy loading</summary>

A page that was never touched is not present – `mprotect` on it fails here; touch it first (or load it in mprotect).
</details>

<details><summary>3 – TLB</summary>

Rights are removed – flush, inside the critical section.
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why can't x86 express 'write-only'?</summary>

x86 page rights are a ladder: **present** means readable; `writeable` adds write; `execution_disabled` (NX) removes execute. There is no bit for "not readable but writable" – any present page can be read. So `PROT_WRITE` alone becomes read + write (POSIX allows giving more rights than asked).

</details>

<details><summary>Why check all pages before changing any?</summary>

All or nothing: if page 5 of 10 isn't mapped and you'd already changed pages 1–4, you'd return an error with half the range changed – the caller can't know which part. Check every page first (under the lock), then change all – the lock guarantees nothing changes in between.

</details>

<details><summary>What does W⊕X protect against?</summary>

Code injection: an attacker writes machine code into a buffer (stack, heap) and makes the program jump there. With W⊕X a page is never both writable and executable – injected data can't be run, and code can't be modified. Attackers then need to reuse existing code (ROP).

</details>
