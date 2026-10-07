#!/usr/bin/env bash
# task.sh - work on the AG training tasks in your training copy of SWEB (repos/sweb-ag).
#
#   task.sh setup                      create the training repo (once; needs your practice repo or internet)
#   task.sh list                       all tasks, with what you have started
#   task.sh start a0-01                fresh branch task/a0-01-... from the right base, tests copied in
#   task.sh test [a0-01]               build the current branch and run the task's tests (headless)
#   task.sh reset a0-01                start the task over - your attempt is kept in a backup/... branch
#   task.sh reset a1                   the same for every started task of A1 (a0, a1, a2, ex, all)
#   task.sh solution a0-01             branch solution/a0-01-... with the reference solution, switched to
#   task.sh diff a0-01                 your attempt vs. the solution (kernel + libc, without tests)
#   task.sh example ex02               branch example/ex02-... with the example applied
#   task.sh run [qemu|kvm]             build the current branch and boot SWEB in a window (like sweb-practise)
#   task.sh debug                      debug build + boot waiting for gdb; then in a 2nd terminal: task.sh gdb
#   task.sh gdb [cgdb|gdb|ddd]         attach the debugger (make runcgdb / rungdb / runddd)
#   task.sh nodebug                    back to the normal build (make x86_64)
#   task.sh status                     current branch, task, uncommitted changes, backups
#
# Everything happens in $SWEB_AG (default ~/Documents/University/2026-fall/os/repos/sweb-ag),
# built in $SWEB_AG_BUILD (default /tmp/sweb-ag). Your practice repo and the team repo are never touched.
# Branches: main = upstream SWEB, base/<id> = main + a base task's solution, task/<id>-<name> = your work,
# solution/<id>-<name>, example/<id>-<name>, backup/<id>-<name>-<date>.

set -euo pipefail

TOOLS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COURSE="$(dirname "$TOOLS")"
REPO="${SWEB_AG:-$HOME/Documents/University/2026-fall/os/repos/sweb-ag}"
BUILD="${SWEB_AG_BUILD:-/tmp/sweb-ag}"
MANIFEST="$TOOLS/tasks.txt"      # id|folder|base|runner options|commands separated by ;

die() { echo "task.sh: $*" >&2; exit 1; }
info() { echo "== $*"; }
g() { git -C "$REPO" "$@"; }

cmd_setup() {
  if [ -d "$REPO/.git" ]; then info "training repo already there: $REPO"; return; fi
  local practice="$HOME/Documents/University/2026-fall/os/repos/sweb"
  mkdir -p "$(dirname "$REPO")"
  if [ -d "$practice/.git" ] && git -C "$practice" cat-file -e f6fcb2ab 2>/dev/null; then
    info "cloning upstream SWEB (f6fcb2ab) from your practice repo $practice"
    git clone -q "$practice" "$REPO"
    git -C "$REPO" switch -q -C main f6fcb2ab
  else
    info "cloning upstream SWEB from GitHub"
    git clone -q https://github.com/isec-tugraz/sweb.git "$REPO"
    git -C "$REPO" switch -q -C main f6fcb2ab
  fi
  git -C "$REPO" remote set-url origin https://github.com/isec-tugraz/sweb.git
  info "training repo ready: $REPO (branch main = upstream f6fcb2ab)"
}

if [ "${1:-}" = setup ]; then cmd_setup; exit 0; fi
[ -d "$REPO/.git" ] || die "training repo $REPO not found - create it with: task.sh setup"

# ---------------------------------------------------------------- manifest
# lookup ID -> T_ID T_DIR T_BASE T_OPTS T_CMDS T_NAME. Accepts a0-01, a0/01, a0-01-meminfo, ex02, ...
lookup() {
  local want="${1//\//-}" line id dir base opts cmds
  while IFS='|' read -r id dir base opts cmds; do
    [[ -z "$id" || "$id" == \#* ]] && continue
    local name="${dir##*/}"
    if [[ "$want" == "$id" || "$want" == "$id-${name#*-}" || "$want" == "${dir//\//-}" ]]; then
      T_ID="$id"; T_DIR="$dir"; T_BASE="$base"; T_OPTS="$opts"; T_CMDS="$cmds"
      T_NAME="$id-${name#*-}"                            # a0-01-meminfo, ex02-map-page
      [[ "$id" == ex* ]] && T_NAME="$name"
      return 0
    fi
  done < "$MANIFEST"
  die "unknown task '$1' (task.sh list)"
}

all_ids() { awk -F'|' '!/^#/ && NF { print $1 }' "$MANIFEST"; }

# ---------------------------------------------------------------- git helpers
branch_exists() { g show-ref --verify --quiet "refs/heads/$1"; }
current_branch() { g symbolic-ref --quiet --short HEAD 2>/dev/null || echo "(detached)"; }
is_dirty() { [ -n "$(g status --porcelain)" ]; }

require_clean() {
  if is_dirty; then
    g status --short | head -10
    die "you have uncommitted changes on $(current_branch) - commit them first (git add -A && git commit -m wip),
         or throw them away with: task.sh reset <task>  (keeps a backup)"
  fi
}

# commit_on PARENT MESSAGE PATCH TESTFILE... -> prints a new commit = PARENT + patch + test files.
# Built in a temporary index: the working tree and the current branch are not touched.
commit_on() {
  local parent="$1" message="$2" patch="$3"; shift 3
  local index; index="$(mktemp)"
  rm -f "$index"
  GIT_INDEX_FILE="$index" g read-tree "$parent"
  if [ -n "$patch" ]; then
    GIT_INDEX_FILE="$index" g apply --cached --whitespace=nowarn "$patch" || { rm -f "$index"; die "patch does not apply: $patch"; }
  fi
  local f
  for f in "$@"; do
    local blob; blob="$(g hash-object -w "$f")"
    GIT_INDEX_FILE="$index" g update-index --add --cacheinfo "100644,$blob,userspace/tests/$(basename "$f")"
  done
  local tree; tree="$(GIT_INDEX_FILE="$index" g write-tree)"
  rm -f "$index"
  g commit-tree "$tree" -p "$parent" -m "$message"
}

tests_of() { ls "$COURSE/$1"/*.c 2>/dev/null || true; }

# the commit a task starts from: main, or base/<id> (created on first use)
base_ref() {
  local base="$1"
  [ "$base" = main ] && { echo main; return; }
  local saved_id="$T_ID" saved_dir="$T_DIR" saved_base="$T_BASE" saved_opts="$T_OPTS" saved_cmds="$T_CMDS" saved_name="$T_NAME"
  lookup "$base"
  if ! branch_exists "base/$T_ID"; then
    local c; c="$(commit_on main "base: solution of $T_NAME" "$COURSE/solutions/$T_DIR/solution.patch" $(tests_of "$T_DIR"))"
    g branch "base/$T_ID" "$c"
    info "created base/$T_ID (main + solution of $T_NAME)" >&2
  fi
  local ref="base/$T_ID"
  T_ID="$saved_id"; T_DIR="$saved_dir"; T_BASE="$saved_base"; T_OPTS="$saved_opts"; T_CMDS="$saved_cmds"; T_NAME="$saved_name"
  echo "$ref"
}

backup_branch() {   # backup_branch BRANCH: commit uncommitted work if it is checked out, keep it as backup/...
  local branch="$1" name="${1#*/}"
  if [ "$(current_branch)" = "$branch" ] && is_dirty; then
    g add -A
    g commit -q -m "wip: uncommitted work saved by task.sh reset"
  fi
  local backup="backup/$name-$(date +%Y%m%d-%H%M%S)"
  g branch "$backup" "$branch"
  info "your work is kept in $backup"
}

# ---------------------------------------------------------------- commands
cmd_list() {
  printf "%-6s %-30s %-7s %s\n" "id" "folder" "base" "state"
  local id dir base opts cmds
  while IFS='|' read -r id dir base opts cmds; do
    [[ -z "$id" || "$id" == \#* ]] && continue
    lookup "$id"
    local state=""
    local work="task/$T_NAME"; [[ "$id" == ex* ]] && work="example/$T_NAME"
    branch_exists "$work" && state="started"
    branch_exists "solution/$T_NAME" && state="${state:+$state, }solution branch"
    [ "$(current_branch)" = "$work" ] && state="$state  <- current"
    printf "%-6s %-30s %-7s %s\n" "$id" "$dir" "$base" "$state"
  done < "$MANIFEST"
}

start_branch() {   # creates task/<name> from the base + tests, switches to it
  local ref; ref="$(base_ref "$T_BASE")"
  local c; c="$(commit_on "$ref" "start $T_NAME: test programs" "" $(tests_of "$T_DIR"))"
  g branch "task/$T_NAME" "$c"
  g switch -q "task/$T_NAME"
  info "on branch task/$T_NAME (based on $ref)"
  info "task card: $COURSE/$T_DIR/README.md"
  info "test it:   task.sh test"
}

cmd_start() {
  lookup "$1"
  [[ "$T_ID" == ex* ]] && die "examples: task.sh example $T_ID"
  if branch_exists "task/$T_NAME"; then
    [ "$(current_branch)" = "task/$T_NAME" ] && { info "already on task/$T_NAME"; return; }
    require_clean
    g switch -q "task/$T_NAME"
    info "switched to task/$T_NAME (started before - task.sh reset $T_ID to begin again)"
    return
  fi
  require_clean
  start_branch
}

cmd_reset() {
  local what="$1" ids=()
  case "$what" in
    a0|a1|a2|ex) mapfile -t ids < <(all_ids | grep "^$what") ;;
    all) mapfile -t ids < <(all_ids) ;;
    *) lookup "$what"; ids=("$T_ID") ;;
  esac
  local single=$(( ${#ids[@]} == 1 ))
  local id
  for id in "${ids[@]}"; do
    lookup "$id"
    local work="task/$T_NAME"; [[ "$T_ID" == ex* ]] && work="example/$T_NAME"
    branch_exists "$work" || continue
    backup_branch "$work"
    if [ "$(current_branch)" = "$work" ]; then
      g switch -q main
    fi
    g branch -q -D "$work"
    info "reset $T_NAME"
    if [ "$single" = 1 ]; then
      if [[ "$T_ID" == ex* ]]; then cmd_example "$T_ID"; else start_branch; fi
    fi
  done
  [ "$single" = 1 ] || info "done - start a task again with task.sh start <id>"
}

make_solution() {
  local patch="$COURSE/solutions/$T_DIR/solution.patch"
  local c; c="$(commit_on main "solution of $T_NAME" "$patch" $(tests_of "$T_DIR"))"
  if [ "$T_BASE" != main ]; then   # also the base task's tests
    local saved_dir="$T_DIR" base_dir
    base_dir="$(awk -F'|' -v b="$T_BASE" '$1 == b { print $2 }' "$MANIFEST")"
    c="$(commit_on "$c" "tests of the base $T_BASE" "" $(tests_of "$base_dir"))"
    T_DIR="$saved_dir"
  fi
  g branch -f "solution/$T_NAME" "$c" >/dev/null
}

cmd_solution() {
  lookup "$1"
  [[ "$T_ID" == ex* ]] && die "examples are complete already: task.sh example $T_ID"
  [ "$(current_branch)" = "solution/$T_NAME" ] || require_clean
  [ "$(current_branch)" = "solution/$T_NAME" ] && g switch -q main
  make_solution
  g switch -q "solution/$T_NAME"
  info "on branch solution/$T_NAME - the reference solution, ready to build: task.sh test / task.sh run"
  info "notes: $COURSE/solutions/$T_DIR/NOTES.md"
  info "back to your attempt: task.sh start $T_ID"
}

cmd_diff() {
  lookup "$1"
  branch_exists "task/$T_NAME" || die "task $T_ID not started"
  make_solution
  g diff "task/$T_NAME" "solution/$T_NAME" -- . ':(exclude)userspace/tests'
}

cmd_example() {
  lookup "$1"
  [[ "$T_ID" == ex* ]] || die "$1 is a task, not an example"
  if branch_exists "example/$T_NAME"; then
    [ "$(current_branch)" = "example/$T_NAME" ] || { require_clean; g switch -q "example/$T_NAME"; }
  else
    require_clean
    local c; c="$(commit_on main "example $T_NAME" "$COURSE/$T_DIR/example.patch" $(tests_of "$T_DIR"))"
    g branch "example/$T_NAME" "$c"
    g switch -q "example/$T_NAME"
  fi
  info "on branch example/$T_NAME - walkthrough: $COURSE/$T_DIR/README.md"
}

id_of_current_branch() {
  local b; b="$(current_branch)"
  case "$b" in
    task/*|solution/*|example/*) local rest="${b#*/}"; echo "${rest%%-*}-$(echo "$rest" | cut -d- -f2)" | sed 's/^\(ex[0-9]*\)-.*/\1/' ;;
    *) echo "" ;;
  esac
}

build() {
  mkdir -p "$BUILD"
  cd "$BUILD"
  cmake "$REPO" > cmake.log 2>&1 || { tail -20 cmake.log; die "cmake failed"; }
  # twice: a parallel build can copy the user programs to the disk image before they are built
  local ok=0
  { make -j"$(nproc)" > make.log 2>&1 && make -j"$(nproc)" >> make.log 2>&1; } && ok=1
  if [ "$ok" = 0 ] && grep -a -q "exe2minixfs.*Assertion" make.log; then
    # the image keeps every program ever built into it - after many tasks its root directory is full
    info "disk image full of old programs - recreating it"
    rm -f SWEB-flat.vmdk SWEB.qcow2
    { make -j"$(nproc)" > make.log 2>&1 && make -j"$(nproc)" >> make.log 2>&1; } && ok=1
  fi
  if [ "$ok" = 0 ]; then
    grep -a -E "error|Error" make.log | head -30
    die "build failed (full log: $BUILD/make.log)"
  fi
}

cmd_test() {
  local id="${1:-$(id_of_current_branch)}"
  [ -n "$id" ] || die "which task? (task.sh test a0-01) - the current branch is not a task branch"
  lookup "$id"
  local opts=() cmds=()
  read -r -a opts <<< "$T_OPTS"
  IFS=';' read -r -a cmds <<< "$T_CMDS"
  info "testing $T_NAME on $(current_branch)"
  SWEB_SRC="$REPO" SWEB_BUILD="$BUILD" "$TOOLS/run_test.sh" "${opts[@]}" "${cmds[@]}"
}

cmd_run() {
  local target="${1:-qemu}"
  build
  info "booting $(current_branch) - press Enter at the GRUB menu, Ctrl+C here to stop"
  make "$target"
}

cmd_status() {
  info "repo:   $REPO"
  info "branch: $(current_branch)   task: $(id_of_current_branch)"
  if is_dirty; then g status --short | head -20; else info "no uncommitted changes"; fi
  local backups; backups="$(g branch --list 'backup/*' | wc -l)"
  info "$backups backup branch(es) - git -C $REPO branch --list 'backup/*'"
}

case "${1:-}" in
  list) cmd_list ;;
  start) [ $# -ge 2 ] || die "task.sh start <id>"; cmd_start "$2" ;;
  reset) [ $# -ge 2 ] || die "task.sh reset <id>|a0|a1|a2|ex|all"; cmd_reset "$2" ;;
  solution) [ $# -ge 2 ] || die "task.sh solution <id>"; cmd_solution "$2" ;;
  diff) [ $# -ge 2 ] || die "task.sh diff <id>"; cmd_diff "$2" ;;
  example) [ $# -ge 2 ] || die "task.sh example <exNN>"; cmd_example "$2" ;;
  test) shift; cmd_test "${1:-}" ;;
  run) shift; cmd_run "${1:-qemu}" ;;
  kvm) cmd_run kvm ;;
  debug) build; make debug > /dev/null && make -j"$(nproc)" > make.log 2>&1 && make -j"$(nproc)" >> make.log 2>&1
         info "debug build - SWEB waits for the debugger. In a second terminal: task.sh gdb"; make qemugdb ;;
  gdb) cd "$BUILD" && make "run${2:-cgdb}" ;;
  nodebug) cd "$BUILD" && make x86_64 > /dev/null && info "normal (non-debug) build configured" ;;
  status) cmd_status ;;
  -h|--help|help|"") sed -n '2,25p' "$0" | sed 's/^# \{0,1\}//' ;;
  *) die "unknown command '$1' (task.sh help)" ;;
esac
