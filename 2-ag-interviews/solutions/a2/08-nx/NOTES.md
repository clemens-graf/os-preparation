# A2-08 NX for data and stack – solution notes

Patch: [`solution.patch`](solution.patch).

- `Loader::loadPage`: `executable |= p_flags & PF_X` for every segment on the page; if not
  executable → `execution_disabled = 1` in the same critical section as `mapPage`.
- `UserProcess` constructor: the stack page gets NX (no lock needed: the process does not run yet).
- `PageFaultHandler::checkPageFaultIsValid`: `present && fetch` → "Instruction fetch from a
  non-executable page (NX)".

Why it works at all: `boot.32.C` enables `EFER.NXE`; without it bit 63 of a PTE is reserved and
setting it causes a reserved-bit page fault (SWEB asserts on that).

Same family: A0-05 (read-only code via PF_W), A2-07 (mprotect).
