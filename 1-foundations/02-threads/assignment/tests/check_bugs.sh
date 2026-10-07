#!/usr/bin/env bash
# Runs the three (hopefully fixed) bug programs several times each and
# checks their output and that no sanitizer complained.
root="$1"
fail=0

check() {   # check <name> <binary> <expected output> <tsan|asan>
  local name="$1" bin="$2" want="$3" mode="$4" got rc
  for run in 1 2 3 4 5; do
    if [ "$mode" = tsan ]; then
      got=$("$root/common/run.sh" --tsan "$bin" 2>build/$name.err)
    else
      got=$("$root/common/run.sh" "$bin" 2>build/$name.err)
    fi
    rc=$?
    if [ $rc -ne 0 ] || [ "$got" != "$want" ]; then
      echo "[ FAIL ] $name (run $run): exit code $rc"
      echo "         expected: $want"
      echo "         got:      $got"
      grep -m3 -E "WARNING|ERROR|SUMMARY" build/$name.err | sed 's/^/         /'
      echo "         full sanitizer output: build/$name.err"
      fail=$((fail+1))
      return
    fi
  done
  echo "[  OK  ] $name (5 runs)"
}

check bug1_loop_index    build/bug1 "squares: 0 1 4 9 16 25 36 49"       tsan
check bug2_dead_arguments build/bug2 "total: 56"                          asan
check bug3_no_join       build/bug3 "sum of 1..4000000 = 8000002000000"  tsan

echo "---- bugs: $((3-fail)) fixed, $fail still broken"
[ $fail = 0 ]
