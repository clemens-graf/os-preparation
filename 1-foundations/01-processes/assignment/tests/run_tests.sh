#!/usr/bin/env bash
# Feeds every tests/cases/*.in file to minish and compares
#   stdout      with <case>.out   (exactly)
#   stderr      with <case>.err   (every line of .err must appear somewhere)
#   exit code   with <case>.code  (default 0)
# Cases whose name starts with "bonus_" test the optional pipe feature and
# do not make `make test` fail.

bin="$1"
cases=tests/cases
out=build/out
mkdir -p "$out"

pass=0; fail=0; bpass=0; bfail=0

for input in "$cases"/*.in; do
  name=$(basename "$input" .in)
  HOME=/ timeout --foreground 10 "$bin" < "$input" > "$out/$name.stdout" 2> "$out/$name.stderr"
  code=$?
  ok=1; why=""

  if [ "$code" = 124 ]; then
    ok=0; why="timeout (minish hangs - waiting for a child that never ends? unclosed pipe end?)"
  fi
  if ! diff -u "$cases/$name.out" "$out/$name.stdout" > "$out/$name.diff"; then
    ok=0; why="$why stdout differs"
  fi
  if [ -f "$cases/$name.err" ]; then
    while IFS= read -r line; do
      if ! grep -qF -- "$line" "$out/$name.stderr"; then
        ok=0; why="$why; stderr lacks: '$line'"
      fi
    done < "$cases/$name.err"
  fi
  want=0
  [ -f "$cases/$name.code" ] && want=$(cat "$cases/$name.code")
  if [ "$code" != 124 ] && [ "$code" != "$want" ]; then
    ok=0; why="$why; exit code $code, expected $want"
  fi

  if [ $ok = 1 ]; then
    echo "[  OK  ] $name"
    if [[ $name == bonus_* ]]; then bpass=$((bpass+1)); else pass=$((pass+1)); fi
  else
    echo "[ FAIL ] $name: $why"
    echo "         input:    $input"
    echo "         yours:    $out/$name.stdout / .stderr"
    if [ -s "$out/$name.diff" ]; then sed 's/^/         /' "$out/$name.diff" | head -20; fi
    if [[ $name == bonus_* ]]; then bfail=$((bfail+1)); else fail=$((fail+1)); fi
  fi
done

echo "---- core: $pass passed, $fail failed | bonus (pipes): $bpass passed, $bfail failed"
[ $fail = 0 ]
