# A2-11 · boot counter + message of the day on the swap device

**Time box 40 min · ★★☆ · read first: ex08**

```
Name:         store the number of boots on the swap device; message of the day
Nr:           1790 (bootcount), 1791 (getmotd), 1792 (setmotd)
Description:  At every boot the kernel reads a record from the first block of idea2
              (magic, boot counter, message of the day), increments the counter, writes it
              back and prints "This is boot number N. Message of the day: ...".
              setmotd(text) stores a message for the next boots (max 255 chars).
```

## Test

`task.sh start a2-11` → `task.sh test`: boots **twice on the same disk** (`-n 2 --keep-disk`).
Done when boot 1 sees no message and boot 2 sees "left by boot 1". (`run_test.sh` normally boots
with a throw-away disk – `--keep-disk` keeps one copy for all runs.)

## Your team's code

Nothing team-specific: `BootRecord` (new), `init()` from `ProcessRegistry::Run()` (needs a thread for disk I/O), three syscalls.

## Hints

<details><summary>1 – when to read the disk</summary>

Disk I/O waits for the IDE interrupt and yields: it needs a running thread with interrupts on.
`startup()` is too early – do it at the start of `ProcessRegistry::Run()`, before the first program.
</details>

<details><summary>2 – one block</summary>

A 512-byte struct with a magic number: a fresh (all zero) disk has no magic → counter starts at 0.
</details>

<details><summary>3 – by hand</summary>

`task.sh run` rebuilds the image every time (counter back to 1). To see it count: boot twice with `make qemu` in the build folder without rebuilding.
</details>

## Questions a tutor might ask

Answer each one out loud first – then open it to compare.

<details><summary>Why can't the disk be read in startup()?</summary>

`readData` waits for the IDE interrupt by **yielding** – that needs a running thread with interrupts on and a working scheduler. In `startup()` the scheduler isn't running yet, there's no thread to switch to. `ProcessRegistry::Run()` is the first kernel thread – the earliest safe place.

</details>

<details><summary>Your swap slots and this record share the partition – how do you keep them apart?</summary>

Reserve the first block/page for the record: the swap slot area starts after it (slot 0 = the record's page is never handed out), or the `SwapManager` knows an offset. Both must agree on the layout – put it in one place (a constant used by both).

</details>

<details><summary>Two processes call setmotd at once – what does your lock guarantee?</summary>

One Mutex around "copy the message into the record + write the block": the two calls run one after the other – the record and the disk contain exactly one of the two messages, never a mix, and the in-memory record and the disk agree.

</details>
