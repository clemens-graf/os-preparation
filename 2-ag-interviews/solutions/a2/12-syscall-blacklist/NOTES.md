# A2-12 syscall blacklist – solution notes

Patch: [`solution.patch`](solution.patch).

```cpp
// top of syscallException
if (currentThread->loader_ && currentThread->loader_->isBlacklisted(syscall_number))
  return (size_t) -1;

bool Loader::isBlacklisted(size_t n)  { ScopeLock lock(blacklist_lock_); /* search */ }
size_t Loader::addToBlacklist(size_t n)
{
  if (n == sc_exit) return -1;          // the process could never end
  ScopeLock lock(blacklist_lock_); /* add if not there, max 16 */
}
```

- Even the read takes the lock: a concurrent `addToBlacklist` writes the array and the count – an
  unlocked reader could see the new count before the new entry.
- Per process (in the Loader); a forked child would copy the list (seccomp filters are inherited).
- The test's last check: after blacklisting `write`, `printf` cannot print anything – so the test
  "passes" by the absence of a `[FAIL]` line.
