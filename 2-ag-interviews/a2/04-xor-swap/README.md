# A2-04 · XOR-encrypted swap

**Time box 25 min · ★★☆ · builds on A2-01**

```
Name:         encrypted writing-out with XOR, 128-bit key
Nr:           1740 (swappeek, for testing)
Description:  Pages are written to the swap device XOR-encrypted with a 128-bit key that is
              chosen at boot; they are decrypted when swapped in. The disk never contains
              the plain content.
              swappeek(address, buffer16) returns the raw first 16 bytes on disk of a
              swapped-out page (for the test).
```

## Test

`task.sh start a2-04` → `task.sh test`. Done when the raw slot does not contain the plain text and
the page comes back intact (plus all A2-01 checks).

## Hints

<details><summary>1 – encrypt a copy</summary>

Do not XOR the page in place before writing: if the write fails (or the page were shared) its
content would be destroyed. Encrypt a kernel copy, write the copy. Decrypt in place after reading
– the page is not mapped yet then.
</details>

<details><summary>2 – the key</summary>

Two 64-bit words, from `rdtsc` at boot; XOR word i with key[i % 2].
</details>

## Questions a tutor might ask

- Why is repeating XOR with a fixed key weak? (A zero page reveals the key.)
- Where would the key live in a real system?
