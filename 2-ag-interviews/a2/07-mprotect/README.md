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

- Why can't x86 express 'write-only'?
- Why check all pages before changing any?
- What does W⊕X protect against?
