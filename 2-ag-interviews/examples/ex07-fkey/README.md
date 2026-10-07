# ex07 – an F-key that makes a process do something

Pressing **F8** makes the next syscall print information about the process that makes it.
Test: `run_test.sh '!key f8' help` → a `[FKEY] F8 handled by /usr/shell.sweb ...` line.

## Why not do the work directly when the key is pressed?

Keys are handled in `Console::handleKey` (`common/source/console/Console.cpp`), which runs in
the **Console kernel thread**. That thread has the kernel's page tables, not the process's –
user memory of a process is not accessible from there, and touching another thread's state
from there races with that thread. So: **set a flag** in the key handler, and let the target
do the work at a safe point in its own context – here the top of `syscallException`.

```cpp
// Console.cpp
case KEY_F8:
  ArchThreads::testSetLock(Syscall::f8_pending_, 1);   // atomic store
  break;

// Syscall.cpp, top of syscallException
if (f8_pending_ && ArchThreads::testSetLock(f8_pending_, 0))   // atomic exchange: read + clear
  handleF8();                                                    // exactly one syscall handles it
```

`testSetLock(var, value)` is an atomic exchange (returns the old value) – "read and clear" in
one step, so two threads cannot both handle the same key press.

The A2 task list has several "press F3/F7 → do X with all pages of the process" tasks: this
is the pattern (flag in the console, work in the process's context).
