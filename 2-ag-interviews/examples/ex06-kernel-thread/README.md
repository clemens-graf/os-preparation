# ex06 – a kernel thread and a Mutex + Condition handshake

`ticker(n)` starts a kernel thread that prints "tick" n times (every 5 timer ticks) and
sleeps until that thread is done. Test: `ex_kernel_thread.sweb` (3 checks, 5 tick lines in
the log).

## A kernel thread

```cpp
class TickerThread : public Thread
{
  public:
    TickerThread(...) : Thread(0, "TickerThread", Thread::KERNEL_THREAD), ... {}
    void Run() override;      // the thread's body
};

Scheduler::instance()->addNewThread(new TickerThread(...));   // from now on it may run
```

`Run()` returns → `threadStartHack` calls `kill()` → the CleanupThread `delete`s the object.
So: always `new`, never on the stack, never delete it yourself. New `.cpp` files need a
cmake re-run (glob). Own debug flag: one line in `common/include/console/debug.h`.

## The handshake

```cpp
// waiting side (the syscall)
Mutex lock("...");
Condition done_condition(&lock, "...");
bool done = false;
addNewThread(new TickerThread(count, &lock, &done_condition, &done));
lock.acquire();
while (!done)                  // WHILE: re-check the state after every wake-up
  done_condition.wait();       // releases lock while sleeping, re-acquires before returning
lock.release();

// signalling side (end of Run)
lock_->acquire();
*done_ = true;                 // 1. change the state ...
done_condition_->signal();     // 2. ... and signal, both under the mutex
lock_->release();
// 3. do not touch lock_/done_ any more: they live on the waiter's stack
```

## Why exactly like this

- **State + condition variable, never a condition variable alone.** `signal()` before the
  waiter sleeps is lost; the `done` flag is not.
- **`while`, not `if`:** a wake-up does not guarantee that the state is what you wait for.
- **State changes under the mutex** – otherwise the waiter can check `done` (false), get
  preempted, the signaller sets it and signals (nobody waits yet), then the waiter sleeps
  forever: the *lost wake-up*.
- **Lifetime:** the mutex/condition live on the waiting syscall's kernel stack. They are
  valid as long as the waiter has not returned – and it cannot return before the signaller
  released the mutex. The signaller must not use them after `release()`.
  The opposite design (mutex inside the TickerThread object) is a use-after-free: the
  thread object is deleted by the CleanupThread while the waiter may still be inside `wait()`.
- SWEB's `Condition::wait` **asserts** that you hold no other lock while waiting.
