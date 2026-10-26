# A1-02 · pthread_multiple + pthread_join_any

**Time box 45 min · ★★★ · builds on A1-01**

**Theory first:** [concepts.pdf](../../concepts.pdf) §15 a thread's life

**How to work:** start with level 1 only. Open level 2 when you are stuck, level 3 only after the
hints did not help. Level 3 tells you *what* to do step by step – the code you write yourself.

## Level 1 – the goal

```
Name:         pthread_multiple, pthread_join_any
Nr:           1601, 1602
Description:  pthread_multiple(ids, count, start_routine, args) creates count threads,
              thread i runs start_routine(args[i]) (args may be NULL). ALL OR NOTHING:
              if one of them cannot be created, none of them may run and every slot
              taken so far is freed again; return -1.
              pthread_join_any(ids, count, &index, &value) waits until ANY of the given
              threads has ended, joins it, and reports its position in ids.
              If none of the ids is a joinable thread, it returns -1 instead of waiting.
Parameters:   pthread_t* ids, size_t count (1..64), void* (*start_routine)(void*), void** args
              pthread_t* ids, size_t count, size_t* index, void** value_ptr
Return Value: 0 or -1
```

## Test

`task.sh start a1-02` → `task.sh test`. Done when all 10 checks pass. The test fills 60 of the
64 places with ended-but-unjoined threads, so that creating 8 must fail half-way.

## Level 2 – hints

<details><summary>1 – all or nothing - how?</summary>

Create every thread object first (they are not in the scheduler yet, so they cannot run),
and only when all exist add them to the scheduler. If creation number k fails, delete the
k-1 objects you created (not scheduled → you may delete them directly) and everything
`createThread` set up for them.
</details>

<details><summary>2 – deleting a thread that never ran</summary>

Undo what `createThread` did, under `thread_lock_`: free its stack (`freeUserThreadStack`),
remove its `UserThreadData` from `threads_` and delete it, `running_threads_--`. Then
`setRegisteredInProcess(false)` (else its destructor calls `threadExited()` again) and `delete` it
(`discardThread`).
</details>

<details><summary>3 – join_any without spinning</summary>

Same Condition as join (`thread_exited_`, broadcast in `exitThread`). Loop: look for one
among the ids with `exited_` set; none → is any of them still joinable (known, not `joined_`)?
no → -1; yes → wait. All under `thread_lock_`.
</details>

## Level 3 – step by step (no code)

<details><summary>Open only if the hints were not enough</summary>

1. Give `createThread` a limit (64 not yet joined threads) so a failure can happen; add `discardThread` to `UserProcess` that undoes `createThread` for a thread that never ran (stack, record, counter, then delete it).
2. Syscall `pthread_multiple(ids, count, start_routine, args)` (1601): check the pointers; copy the `args` array into the kernel first.
3. Create **all** threads first with `createThread` – none of them goes to the scheduler yet.
4. If creation number k fails: discard the k-1 already created ones, return -1.
5. All created: write the ids to the user array, then add every thread to the scheduler.
6. Syscall `pthread_join_any(ids, count, &index, &value)` (1602): copy the ids in; under `thread_lock_` loop: one of them exited → join it (value, record) and remember its position; none of them joinable any more → -1; else wait on `thread_exited_`.
7. Write index and value to the user after releasing the lock. `task test`.

</details>

## Your team's code

Your team's `createThread` has no limit, so the rollback could never happen. The team solution
adds one: **`MAX_USER_THREADS 64`** threads per process that are not joined yet (`threads_.size()`
in `createThread`, under `thread_lock_` – a finished thread's `UserThreadData` stays in `threads_`
until it is joined). `createThread` does not schedule the thread – `pthreadCreate` calls
`addNewThread` afterwards – which is exactly what all-or-nothing needs.

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why may you delete the threads in the rollback yourself, but never a thread that has run?</summary>

A thread that was never added to the scheduler has never run: it holds no lock, is in no waiters list, nothing else points to it – undoing `createThread` (free its stack, remove + delete its record, `running_threads_--`) and `delete` is safe. A thread that ran may be anywhere (holding a lock, sleeping on a Condition, inside the scheduler's list) – it can only end itself (`kill()` → CleanupThread).

</details>

<details><summary>Why copy the args array into the kernel before creating anything?</summary>

The user's array may change while you create threads (another thread of the process writes it – TOCTOU), and the user memory may page-fault while you hold `thread_lock_`. Copy once into a kernel array, check the copy, then work only with it.

</details>

<details><summary>join_any with ids of threads that are all joined already – why must it not wait?</summary>

Waiting only makes sense if one of them can still end and be joined. If all are already joined (unknown records) or being joined by others, nobody will ever broadcast for them – waiting would block forever. Return -1 instead.

</details>

<details><summary>What does 'all or nothing' cost you if the scheduler were added to in the loop instead?</summary>

If each thread is added to the scheduler right after it's created, the first threads already **run** when creation number k fails – they can't be undone any more (see the first question). "All or nothing" then becomes "some started, some not". Creating all first and scheduling them only at the end makes the rollback possible.

</details>
