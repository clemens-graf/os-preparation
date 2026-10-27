# A2-05 · don't write unchanged pages

**Time box 40 min · ★★★ · type E ([TRIAGE](../../TRIAGE.md)) · builds on A2-01**

```
Name:         not writing unchanged pages
Nr:           1750 (swapwrites, for testing)
Description:  When a page is swapped in, its copy on disk stays valid (the slot is kept).
              If the page is swapped out again WITHOUT having been written in between, it
              is not written to disk again - the old slot is reused. If it was written,
              the old slot is overwritten (no new slot).
              swapwrites() returns the number of page writes to the device so far.
```

## Test

`task.sh start a2-05` → `task.sh test`. Done when 7 checks pass: first swap-out writes, a clean one
does not, a dirty one does and reuses its slot, slots freed at exit.

## Your team's code

Builds on A2-01: the changes are in `UserProcess::swapOut/swapIn` (team) and `SwapManager`.

## Hints

<details><summary>1 – how do you know it was written?</summary>

The CPU sets the `dirty` bit in the PTE on every write. Clear it when you swap the page in.
</details>

<details><summary>2 – where is the slot while the page is present?</summary>

`page_ppn` holds the physical page now. The 11 `ignored_1` bits are free: store slot + 1 there
(0 = no copy) – which limits the device to 2047 slots in this solution.
</details>

<details><summary>3 – when to read the dirty bit</summary>

After `present = 0` + TLB flush. Before that another thread could still write and set it – you would
skip the write and lose that change.
</details>

<details><summary>4 – exit</summary>

Present pages with a clean copy own a slot too – free those at exit as well.
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why must the dirty bit be read after the TLB flush?</summary>

The CPU sets the dirty bit on the first write **through the TLB entry**. As long as the page is present, a write can come between "read dirty" and "present = 0" and set the bit after you decided "clean" – that write would be lost. After `present = 0` + flush no write is possible any more, so the bit is final.

</details>

<details><summary>What does the TLB have to do with the dirty bit?</summary>

The CPU caches PTEs in the TLB. It sets the dirty bit in the PTE in memory only the first time it writes through a TLB entry that doesn't have it set yet. If you clear `dirty` in memory while a TLB entry with dirty already set exists, later writes go through that entry and the bit is never set again – so: clear it only when no TLB entry exists (not present / after a flush).

</details>

<details><summary>What would the accessed bit be good for (A2 page replacement: clock / second chance)?</summary>

The CPU sets `accessed` on every use. A replacement policy uses it to find pages that weren't used recently: **clock / second chance** walks the pages, clears `accessed` of a used one (second chance) and evicts the first one whose bit is still clear. Cheap approximation of LRU.

</details>
