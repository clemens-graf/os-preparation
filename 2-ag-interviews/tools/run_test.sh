#!/usr/bin/env bash
# run_test.sh - build SWEB and run commands in it headless.
#
#   tools/run_test.sh meminfo_basic.sweb                  # one test program
#   tools/run_test.sh magic_page_basic.sweb magic_page_write.sweb
#   tools/run_test.sh -n 5 pthread_create_basic.sweb      # 5 runs: races show up
#   tools/run_test.sh --raw upcall_basic.sweb             # the whole kernel log, not just the summary
#   tools/run_test.sh pthread_create_basic.sweb exit      # 'exit' at the end: leak check
#   tools/run_test.sh -n 2 --keep-disk bootcount_basic.sweb  # 2 boots on the same disk
#   tools/run_test.sh "&fkey_swap_check.sweb" "!sleep 1" "!key f3" help   # press a key while a program runs
#   tools/run_test.sh --expect-panic ex_tutorial2.sweb    # a demonstrated bug: the panic is the expected result
#   tools/run_test.sh --expect-pass 23 alias_basic.sweb   # also fail if fewer than 23 [PASS] lines appear
#                                                         # (a test program killed half-way prints no [FAIL])
#
# Source: $SWEB_SRC   (default ~/Documents/University/2026-fall/os/repos/sweb-ag, the training copy)
# Build:  $SWEB_BUILD (default /tmp/sweb-ag)
# Usually you call it through task.sh test, which knows each task's commands.
#
# What it does: cmake (always - so new test files are found), make twice (a parallel build can
# copy the user programs to the disk image too early), boot SWEB in QEMU without a window,
# type the commands into the shell, print every [PASS]/[FAIL]/[INFO] line and kernel errors.
# Nothing in your repository is changed.

SRC=${SWEB_SRC:-$HOME/Documents/University/2026-fall/os/repos/sweb-ag}
BUILD=${SWEB_BUILD:-/tmp/sweb-ag}
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

raw=0
want_pass=""
args=()
while [ $# -gt 0 ]; do
  case "$1" in
    --raw) raw=1; shift ;;
    --expect-pass) want_pass="$2"; shift 2 ;;
    -h|--help) sed -n '2,22p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) args+=("$1"); shift ;;
  esac
done
[ ${#args[@]} -gt 0 ] || { echo "usage: run_test.sh [--raw] [-n RUNS] COMMAND..."; exit 2; }

mkdir -p "$BUILD" && cd "$BUILD" || exit 2
echo "== build $SRC -> $BUILD"
cmake "$SRC" > cmake.log 2>&1 || { tail -20 cmake.log; echo "cmake failed"; exit 2; }
build() { make -j"$(nproc)" > make.log 2>&1 && make -j"$(nproc)" >> make.log 2>&1; }
if ! build; then
  if grep -a -q "exe2minixfs.*Assertion" make.log; then
    # the disk image keeps every program ever built into it; after many tasks its root
    # directory is full and exe2minixfs asserts. A fresh image fixes it.
    echo "== disk image full of old programs - recreating it"
    rm -f SWEB-flat.vmdk SWEB.qcow2
  fi
  if ! build; then
    grep -a -E "error|Error" make.log | head -30
    echo "== BUILD FAILED (full log: $BUILD/make.log)"
    exit 2
  fi
fi
grep -a "warning:" make.log | sort -u | head -10

echo "== run: ${args[*]}"
python3 "$HERE/sweb_run.py" -b "$BUILD" -t 30 "${args[@]}" > "$BUILD/run.log" 2>&1
status=$?
if [ $raw = 1 ]; then
  sed 's/\x1b\[[0-9;]*m//g' "$BUILD/run.log"
else
  sed 's/\x1b\[[0-9;]*m//g' "$BUILD/run.log" | grep -a -E \
    "Syscall::write: \[(PASS|FAIL|INFO)\]|sweb_run|KERNEL PANIC|ssertion|Maybe you are|invalid kernel|kernel address in user|even though the address is mapped|non-executable page|access above|heap access|No section refers|EXIT: called|Unimplemented|leaking|\[FKEY|F3:|This is boot|=====|\[TUTORIAL|\[NOTE |General Protection" \
    | sed 's/^.*Syscall::write: //'
fi
pass=$(grep -a -c 'Syscall::write: \[PASS\]' "$BUILD/run.log")
fail=$(grep -a -c 'Syscall::write: \[FAIL\]' "$BUILD/run.log")
echo "== $pass passed, $fail failed, sweb_run status $status (0 ok, 1 timeout/hang, 2 panic, 3 no boot, 4 leak, 5 no panic although expected)"
echo "== full kernel log: $BUILD/run.log"
if [ -n "$want_pass" ] && [ "$pass" -lt "$want_pass" ]; then
  echo "== expected $want_pass [PASS] lines, got $pass - did a test program die half-way? (see EXIT: called above)"
  exit 1
fi
[ "$status" = 0 ] && [ "$fail" = 0 ]
