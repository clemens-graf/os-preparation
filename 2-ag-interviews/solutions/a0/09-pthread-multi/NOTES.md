# A0-09 pthread_multi – solution notes

Patches: [`solution.patch`](solution.patch) (complete, against upstream – includes A0-08) and
[`on-top-of-a0-08.patch`](on-top-of-a0-08.patch) (only the A0-09 part).

## The code

Kernel:

```cpp
size_t Syscall::pthreadMulti(size_t thread_id, size_t functions, size_t arguments, size_t count, size_t entry)
{
  size_t bytes = count * sizeof(size_t);
  if (/* count 0 or > 16, bad pointers */) return (size_t) -1;

  size_t function_copy[PTHREAD_MULTI_MAX];        // 1. copy NOW, into the kernel
  size_t argument_copy[PTHREAD_MULTI_MAX];
  memcpy(function_copy, (void*) functions, bytes);
  memcpy(argument_copy, (void*) arguments, bytes);
  for (size_t i = 0; i < count; ++i)              // 2. check the COPY
    if (function_copy[i] == 0 || function_copy[i] >= USER_BREAK) return (size_t) -1;
  *(size_t*) thread_id = 0;

  UserThread* thread = currentThread->process_->createThread(entry, 0, 0);
  if (!thread) return (size_t) -1;
  thread->user_registers_->rdi = thread->pushToStack(function_copy, bytes);   // 3. onto ITS stack
  thread->user_registers_->rsi = thread->pushToStack(argument_copy, bytes);
  thread->user_registers_->rdx = count;
  *(size_t*) thread_id = thread->getThreadId();
  Scheduler::instance()->addNewThread(thread);    // 4. now it may run
  return 0;
}
```

`UserThread::pushToStack(data, size)`: new address = `(rsp - size) & ~0xF`, copy page by
page through `checkAddressValid` (ident mapping) under the page-table lock, then
`rsp = address - 8`.

libc:

```c
static void pthreadMultiStart(void* (**functions)(void*), void** arguments, size_t count)
{
  for (size_t i = 0; i < count; ++i)
    functions[i](arguments[i]);
  pthread_exit(0);
}
```

## Why copy – twice

- **Copy at all:** the caller's arrays may be on its stack and gone (or overwritten – the
  test does exactly that) long before the new thread reads them.
- **Kernel copy first, then check:** check-then-use on user memory is a TOCTOU race: another
  thread could change a function pointer between your check and your use. Checking the
  kernel copy cannot be raced.
- **Onto the new thread's stack:** it needs the data in *user* memory, and its stack is the
  one place that belongs only to it and lives exactly as long as it does.

## Locking

`pushToStack` writes into page tables' pages → under `arch_memory_lock_`. The thread's
registers are changed without a lock: it is not in the scheduler yet – nobody else can see
it. That changes the moment `addNewThread` runs, so everything is set up before.

## Answers to the tutor questions

- *Only in user space?* `pthread_create(&t, 0, runAll, packedArgs)` with a heap/static copy
  of the arrays – works with a real `pthread_create`, but needs memory management in user
  space (no `malloc` in SWEB's libc!). The kernel version owns the copy on the thread's stack.
- *A function calls `pthread_exit`:* the thread ends, the remaining functions never run.
