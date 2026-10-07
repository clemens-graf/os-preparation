#!/usr/bin/env python3
"""
sweb_run.py - boot a SWEB build headless, type commands into its shell,
collect the kernel's debug output.

    sweb_run.py [-b BUILD_DIR] [-t SECONDS] [-n RUNS] [--log FILE] [--full] COMMAND...

Examples:
    sweb_run.py help
    sweb_run.py -b /tmp/sweb mult.sweb
    sweb_run.py -n 10 -t 30 pthread_create_test.sweb     # races show on the 10th run
    sweb_run.py '!key f8' help                         # press F8, then run 'help'

A last COMMAND 'exit' ends the shell and waits for SWEB's shutdown, which checks for
leaked physical pages (exit status 4 if any).
A COMMAND starting with "!key " presses the given QEMU key names (f1..f12, ret, a, ...)
without waiting for the prompt. "&prog.sweb" types a command without waiting for it to
finish, "!sleep 2" waits two seconds - together: start a program, press a key while it
runs ('&fkey_swap_check.sweb' '!sleep 1' '!key f3' help).

For every run: boots SWEB in QEMU (no window, -snapshot: the disk image is
never changed), presses Enter at the GRUB menu, waits for the shell prompt,
types each COMMAND + Enter and waits for the prompt to come back. Prints
what each command produced on the debug console (colours removed).

Exit status: 0  every command came back to the prompt
             1  a command did not finish within the timeout (hang, deadlock,
                lost wake-up - or the shell itself died)
             2  kernel panic or failed kernel assertion
             3  SWEB did not reach the shell prompt at all
             4  'exit' was given and SWEB reported leaked physical pages
             5  --expect-panic was given, but SWEB did NOT panic
                (with --expect-panic a kernel panic counts as success: for
                examples that demonstrate a bug, e.g. ex12's double free)

Needs qemu-system-x86_64 and a finished build (SWEB.qcow2 in BUILD_DIR).
Nothing here changes your SWEB repository.
"""
import argparse
import os
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time

PROMPT = "SWEB: /> "
DONE = "all processes terminated"
LEAK = "You might be leaking physical memory pages"
PANIC = re.compile(r"KERNEL PANIC|KPANICT")
ANSI = re.compile(r"\x1b\[[0-9;]*m")

KEYS = {" ": "spc", ".": "dot", "/": "slash", "-": "minus", "_": "shift-minus",
        ",": "comma", "=": "equal", ";": "semicolon", "'": "apostrophe",
        "\\": "backslash", "*": "shift-8", "+": "shift-equal", "<": "shift-comma",
        ">": "shift-dot", "!": "shift-1", "?": "shift-slash"}


def key_for(ch):
    if ch.isdigit() or ("a" <= ch <= "z"):
        return ch
    if "A" <= ch <= "Z":
        return "shift-" + ch.lower()
    if ch in KEYS:
        return KEYS[ch]
    raise SystemExit(f"sweb_run: cannot type {ch!r}")


class Sweb:
    def __init__(self, build, workdir, disk=None):
        self.workdir = workdir
        self.log_path = os.path.join(workdir, "debugcon.log")
        image = disk or os.path.join(build, "SWEB.qcow2")
        if not os.path.exists(image):
            raise SystemExit(f"sweb_run: {image} not found - build SWEB first "
                             f"(cmake -B {build} -S <sweb> && cmake --build {build})")
        # The monitor socket path is relative: absolute paths easily exceed
        # the 108-byte limit of Unix sockets.
        self.qemu = subprocess.Popen(
            ["qemu-system-x86_64", "-m", "8M",
             "-drive", f"file={image},index=0,media=disk"] + ([] if disk else ["-snapshot"]) + [
             "-cpu", "qemu64", "-debugcon", "file:debugcon.log",
             "-monitor", "unix:mon.sock,server,nowait",
             "-display", "none", "-no-reboot"],
            cwd=workdir, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        self.mon = socket.socket(socket.AF_UNIX)
        old = os.getcwd()
        os.chdir(workdir)
        try:
            for _ in range(50):
                try:
                    self.mon.connect("mon.sock")
                    break
                except OSError:
                    if self.qemu.poll() is not None:
                        raise SystemExit("sweb_run: qemu did not start: " +
                                         self.qemu.stderr.read().decode())
                    time.sleep(0.1)
            else:
                raise SystemExit("sweb_run: cannot connect to the qemu monitor")
        finally:
            os.chdir(old)

    def log(self):
        try:
            with open(self.log_path, "rb") as f:
                return ANSI.sub("", f.read().decode("latin-1"))
        except FileNotFoundError:
            return ""

    def key(self, name):
        self.mon.sendall(f"sendkey {name}\n".encode())
        time.sleep(0.08)

    def type_line(self, text):
        for ch in text:
            self.key(key_for(ch))
        self.key("ret")

    def wait_for(self, needle, timeout):
        """Wait until the log contains needle. Returns (status, log)."""
        end = time.time() + timeout
        while time.time() < end:
            text = self.log()
            if PANIC.search(text):
                time.sleep(0.5)
                return 2, self.log()
            if needle in text:
                return 0, text
            time.sleep(0.2)
        return 1, self.log()

    def wait_for_prompts(self, count, timeout):
        """Wait until the log shows `count` prompts. Returns (status, log)."""
        end = time.time() + timeout
        while time.time() < end:
            text = self.log()
            if PANIC.search(text):
                time.sleep(0.5)                  # let the backtrace arrive
                return 2, self.log()
            if text.count(PROMPT) >= count:
                return 0, text
            time.sleep(0.2)
        return 1, self.log()

    def close(self):
        try:
            self.mon.sendall(b"quit\n")
        except OSError:
            pass
        try:
            self.qemu.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self.qemu.kill()


def one_run(args, run, disk=None):
    workdir = tempfile.mkdtemp(prefix="sweb_run_")
    sweb = Sweb(args.build, workdir, disk)
    status = 0
    try:
        time.sleep(1.0)
        sweb.key("ret")                          # GRUB has no timeout: pick entry 0
        st, text = sweb.wait_for_prompts(1, args.boot_timeout)
        if st != 0:
            print(text[-3000:])
            print(f"sweb_run: run {run}: SWEB did not reach the shell prompt"
                  + (" (kernel panic)" if st == 2 else ""))
            if "loading /usr/shell.sweb failed" in text:
                print("sweb_run: the shell is missing from the disk image. After adding a user "
                      "program, a parallel build can copy the programs to the image too early: "
                      "build once more (or without -j).")
            return 3 if st == 1 else 2
        pending = None                           # log position where a "&" command started
        for cmd in args.commands:
            if cmd.startswith("!sleep "):               # just wait
                time.sleep(float(cmd[7:]))
                text = sweb.log()
                continue
            if cmd.startswith("&"):                     # type it, don't wait for the prompt
                if pending is None:
                    pending = len(sweb.log())
                sweb.type_line(cmd[1:])
                text = sweb.log()
                continue
            if cmd.startswith("!key "):                 # press keys, don't wait for a prompt
                for k in cmd[5:].split():
                    sweb.key(k)
                time.sleep(0.5)
                text = sweb.log()
                continue
            if cmd.split()[0] in ("exit", "q", "quit"):  # shell ends: wait for SWEB's leak check
                start = len(text) if pending is None else pending
                pending = None
                sweb.type_line(cmd)
                st, text = sweb.wait_for(DONE, args.timeout)
                time.sleep(1.0)
                text = sweb.log()
                print(f"===== run {run}: {cmd} " + "=" * max(0, 50 - len(cmd)))
                print(text if args.full else text[start:])
                if st == 2:
                    print(f"sweb_run: run {run}: KERNEL PANIC during '{cmd}'")
                    status = 2
                elif LEAK in text:
                    print(f"sweb_run: run {run}: physical pages leaked (see the PM lines above)")
                    status = 4
                elif st == 1:
                    print(f"sweb_run: run {run}: SWEB did not shut down after '{cmd}'")
                    status = 1
                break
            before = text.count(PROMPT)
            start = len(text) if pending is None else pending   # include output of "&" commands
            pending = None
            sweb.type_line(cmd)
            st, text = sweb.wait_for_prompts(before + 1, args.timeout)
            print(f"===== run {run}: {cmd} " + "=" * max(0, 50 - len(cmd)))
            print(text if args.full else text[start:])
            if st == 1:
                print(f"sweb_run: run {run}: '{cmd}' did not return to the prompt within "
                      f"{args.timeout} s (hang? deadlock? lost wake-up?)")
                status = 1
                break
            if st == 2:
                print(f"sweb_run: run {run}: KERNEL PANIC during '{cmd}'")
                status = 2
                break
    finally:
        sweb.close()
        if args.log:
            shutil.copy(sweb.log_path, args.log if args.runs == 1 else f"{args.log}.{run}")
        shutil.rmtree(workdir, ignore_errors=True)
    return status


def main():
    p = argparse.ArgumentParser(description=__doc__.split("\n")[1],
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("commands", nargs="+", metavar="COMMAND")
    p.add_argument("-b", "--build", default=os.environ.get("SWEB_BUILD", "/tmp/sweb"),
                   help="SWEB build directory (default: $SWEB_BUILD or /tmp/sweb)")
    p.add_argument("-t", "--timeout", type=float, default=20,
                   help="seconds per command (default 20)")
    p.add_argument("--boot-timeout", type=float, default=30)
    p.add_argument("-n", "--runs", type=int, default=1, help="repeat the whole run N times")
    p.add_argument("--log", help="save the complete debug output here")
    p.add_argument("--full", action="store_true", help="print the whole log, not just the commands")
    p.add_argument("--expect-panic", action="store_true",
                   help="the run is supposed to end in a kernel panic (a demonstrated bug): "
                        "panic -> status 0, no panic -> status 5")
    p.add_argument("--keep-disk", action="store_true",
                   help="all runs use ONE copy of the disk without -snapshot: writes survive to the next run "
                        "(e.g. -n 2 --keep-disk to test a boot counter)")
    args = p.parse_args()

    disk = None
    if args.keep_disk:
        disk = os.path.join(tempfile.mkdtemp(prefix="sweb_disk_"), "SWEB.qcow2")
        shutil.copy(os.path.join(args.build, "SWEB.qcow2"), disk)
    worst = 0
    for run in range(1, args.runs + 1):
        st = one_run(args, run, disk)
        if args.expect_panic:
            if st == 2:
                print(f"sweb_run: run {run}: the kernel panicked - as expected (--expect-panic)")
                st = 0
            elif st == 0:
                print(f"sweb_run: run {run}: expected a kernel panic, but SWEB survived")
                st = 5
        worst = max(worst, st)
        if args.runs > 1:
            print(f"sweb_run: run {run}/{args.runs}: " +
                  ["ok", "TIMEOUT", "KERNEL PANIC", "NO BOOT", "LEAK", "NO PANIC"][st])
    if disk:
        shutil.rmtree(os.path.dirname(disk), ignore_errors=True)
    sys.exit(worst)


if __name__ == "__main__":
    main()
