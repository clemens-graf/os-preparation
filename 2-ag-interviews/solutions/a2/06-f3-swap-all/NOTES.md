# A2-06 F3: swap out all pages of a process – solution notes

Patches: [`on-top-of-a2-01.patch`](on-top-of-a2-01.patch), [`solution.patch`](solution.patch).

```cpp
// Console::handleKey - Console kernel thread, NOT the process' context
case KEY_F3:
  ArchThreads::testSetLock(Syscall::f3_pending_, 1);
  break;

// top of Syscall::syscallException - the process' own context
if (f3_pending_ && ArchThreads::testSetLock(f3_pending_, 0))   // atomic read-and-clear
  currentThread->loader_->swapOutAll();

size_t Loader::swapOutAll()
{
  lock; count = arch_memory_.collectPresentPages(vpns, 512); unlock;   // collect first
  for (each vpn) swapOut(vpn);                                         // each takes the lock itself
}
```

- Why not in `handleKey`: wrong page tables (the console thread has the kernel's), and the
  process may be running/faulting at the same time without any coordination.
- Collect, then act: `swapOut` takes the lock and changes the tables you would be iterating.
- The pages of the running code and stack are swapped out too – they come straight back on the next
  instruction (page fault → swap-in). Correct, just slow.
- Testing it needs a key press *while* the program runs: `run_test.sh '&fkey_swap_check.sweb'
  '!sleep 1' '!key f3' help` (start without waiting, press, then wait for the prompt).
