# A2-04 · XOR-encrypted swap

**Time box 25 min · ★★☆ · type E ([TRIAGE](../../TRIAGE.md)) · builds on A2-01**

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

## Your team's code

Builds on A2-01: the encryption is inside `SwapManager`; `peekSwapped` (testing) in `UserProcess`.

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

Answer each one out loud first – then open it to compare.

<details><summary>Why is repeating XOR with a fixed key weak? (A zero page reveals the key.)</summary>

XOR with a repeating key: `cipher = plain ^ key`. A page of zeros (very common: `.bss`, fresh heap) is stored as **the key itself**. Anyone who can read the disk and guesses one plaintext page learns the key and decrypts everything; identical plaintext blocks also give identical ciphertext blocks. Real systems use a block cipher such as AES-XTS.

</details>

<details><summary>Where would the key live in a real system?</summary>

Never on the disk it protects: generated at boot from a good random source and kept only in kernel memory (a fresh per-boot key for swap is common – swap content doesn't need to survive a reboot). For data at rest: derived from a password or held in a TPM/secure enclave.

</details>
