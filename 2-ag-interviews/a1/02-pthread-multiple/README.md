# A1-02 · pthread_multiple + pthread_join_any

**Time box 45 min · ★★★ · builds on A1-01**

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
64 slots with ended-but-unjoined threads, so that creating 8 must fail half-way.

## Hints

<details><summary>1 – all or nothing - how?</summary>

Create every thread object first (they are not in the scheduler yet, so they cannot run),
and only when all exist add them to the scheduler. If creation number k fails, delete the
k-1 objects you created (not scheduled → you may delete them directly) and free their slots.
</details>

<details><summary>2 – deleting a thread that never ran</summary>

Its destructor unmaps its stack and marks the slot FINISHED (as if it had ended). Nobody
knows its id, so nobody will join it: mark the slot FREE afterwards (`discardThread`).
</details>

<details><summary>3 – join_any without spinning</summary>

Same Condition as join (broadcast in `threadFinished`). Loop: look for a FINISHED one among
the ids; none → are any of them still RUNNING? no → -1; yes → wait. All under the lock.
</details>

## Questions a tutor might ask

- Why may you delete the threads in the rollback yourself, but never a thread that has run?
- Why copy the args array into the kernel before creating anything?
- join_any with ids of threads that are all joined already – why must it not wait?
- What does 'all or nothing' cost you if the scheduler were added to in the loop instead?
