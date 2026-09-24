#!/usr/bin/env bash
# Runs one test binary and translates the exit code into something readable.
#
#   run.sh [--tsan] <binary> [args...]
#
# --tsan: ThreadSanitizer cannot cope with the high ASLR entropy of recent
#         kernels ("FATAL: ThreadSanitizer: unexpected memory mapping").
#         Disabling ASLR for just this process fixes it without root rights.

tsan=0
if [ "$1" = "--tsan" ]; then tsan=1; shift; fi

bin="$1"; shift
limit="${TEST_TIMEOUT:-30}"

if [ ! -x "$bin" ]; then
  echo "run.sh: $bin does not exist (did the build fail?)" >&2
  exit 1
fi

if [ $tsan -eq 1 ]; then
  timeout --foreground "$limit" setarch "$(uname -m)" -R "$bin" "$@"
else
  timeout --foreground "$limit" "$bin" "$@"
fi
rc=$?

case $rc in
  0)   ;;
  124) echo ">>> $bin: TIMEOUT after ${limit}s — almost always a deadlock or a thread that never finishes." >&2 ;;
  66)  echo ">>> $bin: ThreadSanitizer reported a data race (see the report above)." >&2 ;;
  134) echo ">>> $bin: aborted (assert failed or abort() called)." >&2 ;;
  139) echo ">>> $bin: segmentation fault." >&2 ;;
  *)   echo ">>> $bin: exited with code $rc." >&2 ;;
esac
exit $rc
