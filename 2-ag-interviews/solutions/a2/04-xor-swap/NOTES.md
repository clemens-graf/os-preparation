# A2-04 XOR-encrypted swap – solution notes

Patches: [`on-top-of-a2-01.patch`](on-top-of-a2-01.patch), [`solution.patch`](solution.patch).

```cpp
// writeOut: encrypt a COPY
char* encrypted = new char[PAGE_SIZE];
memcpy(encrypted, ident(ppn), PAGE_SIZE);
crypt(encrypted);
device_->writeData(slot * PAGE_SIZE, PAGE_SIZE, encrypted);
delete[] encrypted;

// readIn: decrypt in place (the page is not mapped yet)
device_->readData(..., ident(ppn)); crypt(ident(ppn));

void SwapManager::crypt(char* page)          // XOR is its own inverse
{
  uint64* words = (uint64*) page;
  for (size_t i = 0; i < PAGE_SIZE / 8; ++i)
    words[i] ^= key_[i % 2];
}
```

Key: `rdtsc` at boot, mixed with a constant. `swappeek` reads the raw slot for the test.

Weakness (a good interview answer): with a repeating key, a page of zeros is stored as the key
itself; anyone who can read the disk and guess one plaintext page learns the key. Real systems
use a block cipher (AES-XTS) with a per-boot random key.
