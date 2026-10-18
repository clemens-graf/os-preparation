# A1-02 · pthread_multiple + pthread_join_any

**Time box 45 min · ★★★ · builds on A1-01**

**Theory first:** [concepts.pdf](../../concepts.pdf) §15 a thread's life

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

## Your team's code

Your team's `createThread` has no limit, so the rollback could never happen. The team solution
adds one: **`MAX_USER_THREADS 64`** threads per process that are not joined yet (`threads_.size()`
in `createThread`, under `thread_lock_` – a finished thread's `UserThreadData` stays in `threads_`
until it is joined). `createThread` does not schedule the thread – `pthreadCreate` calls
`addNewThread` afterwards – which is exactly what all-or-nothing needs.

## Hints

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

## Questions a tutor might ask

- Why may you delete the threads in the rollback yourself, but never a thread that has run?
- Why copy the args array into the kernel before creating anything?
- join_any with ids of threads that are all joined already – why must it not wait?
- What does 'all or nothing' cost you if the scheduler were added to in the loop instead?
