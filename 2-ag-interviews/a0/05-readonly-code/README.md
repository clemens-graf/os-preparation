# A0-05 · read-only code (the loader)

**Time box 25 min · ★★☆ · read first: [ex09](../../examples/ex09-loader/), [ex04](../../examples/ex04-pte-flags/)**

```
Name:         read-only code
Description:  Today the loader maps every page of a program writable – a program can
              overwrite its own code. Change the loader so that a page is only writable
              if one of the ELF segments on that page is writable (PF_W in p_flags).
              Writing to code (.text) or constant data (.rodata) must kill the process;
              .data and .bss stay writable.
Parameters:   -
Return Value: -
Notes:        A page can contain parts of more than one segment.
```

## Test

`task start a0-05`, implement, `task test` (runs `readonly_code_basic.sweb`, `readonly_code_rodata.sweb`, `mult.sweb`).
Done when 3 checks pass, both test programs are killed at their write, and `mult.sweb` still works normally.

## Hints

<details><summary>1 – where does the loader map pages?</summary>

`Loader::loadPage` (`common/source/kernel/Loader.cpp`): it loops over `phdrs_` (the
loadable program headers), copies the parts of every segment that intersect the page into
a new physical page, then calls `mapPage`. `readelf -lW /tmp/sweb-ag/userspace/mult.sweb`
shows the segments and their flags (R, W, E).
</details>

<details><summary>2 – the flag</summary>

`p_flags`: 1 = PF_X, 2 = PF_W, 4 = PF_R (no constants in SWEB – define one). The loop has
two branches that set `found_page_content` (file content / only bss) – both have to look
at the flags.
</details>

<details><summary>3 – locking</summary>

As in A0-04: map and clear the writable bit inside the same critical section.
</details>

## Questions a tutor might ask

- Why do you have to look at *all* segments on the page, not just the first one?
- What is the difference between `p_filesz` and `p_memsz`?
- Which page fault does a write to `main` cause, and where is it rejected?
- How would you also make the stack and the heap **non-executable**? (`execution_disabled` bit – A2!)
