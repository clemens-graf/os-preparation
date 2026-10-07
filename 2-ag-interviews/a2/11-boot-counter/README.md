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

- Why can't the disk be read in startup()?
- Your swap slots and this record share the partition – how do you keep them apart?
- Two processes call setmotd at once – what does your lock guarantee?
