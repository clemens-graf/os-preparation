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
#   task.sh team setup                 import your team repo's code (pinned commit) as branch team/main
#   task.sh mode team                  from now on: tasks, solutions, examples on your TEAM code
#   task.sh mode base                  back to base SWEB (the default)
#
# Everything happens in $SWEB_AG (default ~/Documents/University/2026-fall/os/repos/sweb-ag),
# built in $SWEB_AG_BUILD (default /tmp/sweb-ag). Your practice repo and the team repo are never changed
# (team setup only reads $TEAM_REPO, default ~/Documents/University/2026-fall/os/repos/osw26e3).
# Branches: main = upstream SWEB, base/<id> = main + a base task's solution, task/<id>-<name> = your work,
# solution/<id>-<name>, example/<id>-<name>, backup/<id>-<name>-<date>.
# Team mode: the same names under team/ (team/main = your team's code, team/task/a0-01-meminfo, ...).

set -euo pipefail

TOOLS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COURSE="$(dirname "$TOOLS")"
REPO="${SWEB_AG:-$HOME/Documents/University/2026-fall/os/repos/sweb-ag}"
BUILD="${SWEB_AG_BUILD:-/tmp/sweb-ag}"
MANIFEST="$TOOLS/tasks.txt"      # id|folder|base|runner options|commands separated by ;
TEAM_REPO="${TEAM_REPO:-$HOME/Documents/University/2026-fall/os/repos/osw26e3}"
TEAM_PIN="$COURSE/team/BASE"     # the team commit the team versions were made and tested on

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

# ---------------------------------------------------------------- base SWEB or team code
# MODE base: everything starts from main (upstream SWEB), patches from solutions/ and examples/.
# MODE team: everything starts from team/main (your team's code), patches from team/, branches under team/.
# Set with 'task.sh mode team|base' (stored in the training repo's git config). test and status work
# on the current branch, so for them the branch decides (team/... = team mode).
MODE="${TASK_MODE:-$(git -C "$REPO" config --get ag.mode 2>/dev/null || echo base)}"
if [[ "${1:-}" == test || "${1:-}" == status ]]; then
  case "$(git -C "$REPO" symbolic-ref --quiet --short HEAD 2>/dev/null)" in
    team/*) MODE=team ;;
    task/*|solution/*|example/*) MODE=base ;;
  esac
fi
case "$MODE" in
  team) BP="team/"; ROOT="team/main"; PATCHES="$COURSE/team" ;;
  base) BP=""; ROOT="main"; PATCHES="$COURSE" ;;
  *) die "unknown mode '$MODE' (base or team)" ;;
esac

cmd_team_setup() {
  [ -f "$TEAM_PIN" ] || die "$TEAM_PIN missing"
  local pin; pin="$(awk 'NF && !/^#/ { print $1; exit }' "$TEAM_PIN")"
  if git -C "$REPO" show-ref --verify --quiet refs/heads/team/main; then
    if [ "$(git -C "$REPO" rev-parse team/main)" = "$pin" ]; then info "team/main is already your team's code ($pin)"; return; fi
  fi
  [ -d "$TEAM_REPO/.git" ] || die "team repo not found: $TEAM_REPO (set TEAM_REPO=/path/to/your/team/repo)"
  git -C "$TEAM_REPO" cat-file -e "$pin^{commit}" 2>/dev/null ||
    die "your team repo does not have commit $pin yet - update it first: git -C $TEAM_REPO fetch"
  info "importing your team's code ($pin) from $TEAM_REPO - read only, nothing is changed there"
  git -C "$REPO" fetch -q "$TEAM_REPO" "$pin"
  git -C "$REPO" branch -f team/main "$pin" >/dev/null
  info "branch team/main = $(git -C "$REPO" log --oneline -1 team/main)"
  info "switch to team mode with: task.sh mode team"
}

cmd_mode() {
  case "${1:-}" in
    "") info "mode: $MODE (task.sh mode team | task.sh mode base)" ;;
    team)
      git -C "$REPO" show-ref --verify --quiet refs/heads/team/main || die "no team/main yet - run first: task.sh team setup"
      git -C "$REPO" config ag.mode team
      info "team mode: tasks, solutions and examples now use your team's code (branch team/main)" ;;
    base)
      git -C "$REPO" config ag.mode base
      info "base mode: tasks, solutions and examples use base SWEB (branch main)" ;;
    *) die "task.sh mode team|base" ;;
  esac
}

case "${1:-}" in
  team) [ "${2:-}" = setup ] || die "task.sh team setup"; cmd_team_setup; exit 0 ;;
  mode) cmd_mode "${2:-}"; exit 0 ;;
esac

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

# patch_of ID DIR -> the solution/example patch for the current mode (dies if there is no team version)
patch_of() {
  local f
  if [[ "$1" == ex* ]]; then f="$PATCHES/$2/example.patch"; else f="$PATCHES/solutions/$2/solution.patch"; fi
  [ -f "$f" ] || die "$1 has no team version yet (team versions exist for A0 and the examples) - task.sh mode base"
  echo "$f"
}

work_branch() { if [[ "$1" == ex* ]]; then echo "${BP}example/$2"; else echo "${BP}task/$2"; fi; }

# the commit a task starts from: main, or base/<id> (created on first use)
base_ref() {
  local base="$1"
  [ "$base" = main ] && { echo "$ROOT"; return; }
  local saved_id="$T_ID" saved_dir="$T_DIR" saved_base="$T_BASE" saved_opts="$T_OPTS" saved_cmds="$T_CMDS" saved_name="$T_NAME"
  lookup "$base"
  if ! branch_exists "${BP}base/$T_ID"; then
    local patch; patch="$(patch_of "$T_ID" "$T_DIR")"
    local c; c="$(commit_on "$ROOT" "base: solution of $T_NAME" "$patch" $(tests_of "$T_DIR"))"
    g branch "${BP}base/$T_ID" "$c"
    info "created ${BP}base/$T_ID ($ROOT + solution of $T_NAME)" >&2
  fi
  local ref="${BP}base/$T_ID"
  T_ID="$saved_id"; T_DIR="$saved_dir"; T_BASE="$saved_base"; T_OPTS="$saved_opts"; T_CMDS="$saved_cmds"; T_NAME="$saved_name"
  echo "$ref"
}

backup_branch() {   # backup_branch BRANCH: commit uncommitted work if it is checked out, keep it as backup/...
  local branch="$1" name="${1#$BP}"
  if [ "$(current_branch)" = "$branch" ] && is_dirty; then
    g add -A
    g commit -q -m "wip: uncommitted work saved by task.sh reset"
  fi
  local backup="${BP}backup/${name#*/}-$(date +%Y%m%d-%H%M%S)"
  g branch "$backup" "$branch"
  info "your work is kept in $backup"
}

# ---------------------------------------------------------------- commands
cmd_list() {
  info "mode: $MODE (tasks start from $ROOT)"
  printf "%-6s %-30s %-9s %s\n" "id" "folder" "base" "state"
  local id dir base opts cmds
  while IFS='|' read -r id dir base opts cmds; do
    [[ -z "$id" || "$id" == \#* ]] && continue
    lookup "$id"
    local state=""
    local work; work="$(work_branch "$id" "$T_NAME")"
    if [ "$MODE" = team ] && ! ( patch_of "$id" "$dir" >/dev/null 2>&1 ); then state="(no team version)"; fi
    branch_exists "$work" && state="started"
    branch_exists "${BP}solution/$T_NAME" && state="${state:+$state, }solution branch"
    [ "$(current_branch)" = "$work" ] && state="$state  <- current"
    [ "$base" = main ] && base="$ROOT"
    printf "%-6s %-30s %-9s %s\n" "$id" "$dir" "$base" "$state"
  done < "$MANIFEST"
}

start_branch() {   # creates task/<name> from the base + tests, switches to it
  if [ "$MODE" = team ]; then patch_of "$T_ID" "$T_DIR" >/dev/null; fi     # only tasks with a team version
  local ref; ref="$(base_ref "$T_BASE")"
  local c; c="$(commit_on "$ref" "start $T_NAME: test programs" "" $(tests_of "$T_DIR"))"
  g branch "${BP}task/$T_NAME" "$c"
  g switch -q "${BP}task/$T_NAME"
  info "on branch ${BP}task/$T_NAME (based on $ref)"
  if [ "$MODE" = team ]; then info "team code: names differ from the card - read $COURSE/team/README.md"; fi
  info "task card: $COURSE/$T_DIR/README.md"
  info "test it:   task.sh test"
}

cmd_start() {
  lookup "$1"
  [[ "$T_ID" == ex* ]] && die "examples: task.sh example $T_ID"
  if branch_exists "${BP}task/$T_NAME"; then
    [ "$(current_branch)" = "${BP}task/$T_NAME" ] && { info "already on ${BP}task/$T_NAME"; return; }
    require_clean
    g switch -q "${BP}task/$T_NAME"
    info "switched to ${BP}task/$T_NAME (started before - task.sh reset $T_ID to begin again)"
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
    local work; work="$(work_branch "$T_ID" "$T_NAME")"
    branch_exists "$work" || continue
    backup_branch "$work"
    if [ "$(current_branch)" = "$work" ]; then
      g switch -q "$ROOT"
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
  local patch; patch="$(patch_of "$T_ID" "$T_DIR")"
  local c; c="$(commit_on "$ROOT" "solution of $T_NAME" "$patch" $(tests_of "$T_DIR"))"
  if [ "$T_BASE" != main ]; then   # also the base task's tests
    local saved_dir="$T_DIR" base_dir
    base_dir="$(awk -F'|' -v b="$T_BASE" '$1 == b { print $2 }' "$MANIFEST")"
    c="$(commit_on "$c" "tests of the base $T_BASE" "" $(tests_of "$base_dir"))"
    T_DIR="$saved_dir"
  fi
  g branch -f "${BP}solution/$T_NAME" "$c" >/dev/null
}

cmd_solution() {
  lookup "$1"
  [[ "$T_ID" == ex* ]] && die "examples are complete already: task.sh example $T_ID"
  [ "$(current_branch)" = "${BP}solution/$T_NAME" ] || require_clean
  [ "$(current_branch)" = "${BP}solution/$T_NAME" ] && g switch -q "$ROOT"
  make_solution
  g switch -q "${BP}solution/$T_NAME"
  info "on branch ${BP}solution/$T_NAME - the reference solution, ready to build: task.sh test / task.sh run"
  info "notes: $COURSE/solutions/$T_DIR/NOTES.md"
  if [ "$MODE" = team ]; then info "team version: what differs is in $COURSE/team/README.md"; fi
  info "back to your attempt: task.sh start $T_ID"
}

cmd_diff() {
  lookup "$1"
  branch_exists "${BP}task/$T_NAME" || die "task $T_ID not started"
  make_solution
  g diff "${BP}task/$T_NAME" "${BP}solution/$T_NAME" -- . ':(exclude)userspace/tests'
}

cmd_example() {
  lookup "$1"
  [[ "$T_ID" == ex* ]] || die "$1 is a task, not an example"
  if branch_exists "${BP}example/$T_NAME"; then
    [ "$(current_branch)" = "${BP}example/$T_NAME" ] || { require_clean; g switch -q "${BP}example/$T_NAME"; }
  else
    require_clean
    local patch; patch="$(patch_of "$T_ID" "$T_DIR")"
    local c; c="$(commit_on "$ROOT" "example $T_NAME" "$patch" $(tests_of "$T_DIR"))"
    g branch "${BP}example/$T_NAME" "$c"
    g switch -q "${BP}example/$T_NAME"
  fi
  info "on branch ${BP}example/$T_NAME - walkthrough: $COURSE/$T_DIR/README.md"
}

id_of_current_branch() {
  local b; b="$(current_branch)"
  b="${b#team/}"
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
  local cfg; cfg="$(g config --get ag.mode 2>/dev/null || echo base)"
  info "mode:   $cfg (task.sh mode team|base) - this branch is ${MODE} SWEB$([ "$MODE" = team ] && echo " (your team's code)")"
  info "branch: $(current_branch)   task: $(id_of_current_branch)"
  if is_dirty; then g status --short | head -20; else info "no uncommitted changes"; fi
  local backups; backups="$(g branch --list "${BP}backup/*" | wc -l)"
  info "$backups backup branch(es) - git -C $REPO branch --list '${BP}backup/*'"
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
  -h|--help|help|"") sed -n '2,30p' "$0" | sed 's/^# \{0,1\}//' ;;
  *) die "unknown command '$1' (task.sh help)" ;;
esac
