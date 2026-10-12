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
# Tasks run on YOUR TEAM'S CODE (osw26e3, the code you have in the interview). Tasks without a team
# version yet (A1, A2) automatically use base SWEB.
#   task.sh team setup                 (re)import your team's code (pinned commit, team/BASE) - done automatically
#   task.sh mode base                  only if you want base SWEB for everything; task.sh mode team: back
#
# Everything happens in $SWEB_AG (default ~/Documents/University/2026-fall/os/repos/sweb-ag),
# built in $SWEB_AG_BUILD (default /tmp/sweb-ag). Your practice repo and the team repo are never changed
# (the team code is only read from $TEAM_REPO, default ~/Documents/University/2026-fall/os/repos/osw26e3).
# Branches: main = your team's code, task/<id>-<name> = your work, solution/..., example/...,
# base/<id>, backup/...; base SWEB (A1/A2, task.sh mode base) uses the same names under base-sweb/
# (base-sweb/main = upstream SWEB f6fcb2ab).

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

# main = your team's code (the pinned commit in team/BASE, fetched from your team repo - read only)
cmd_team_setup() {
  [ -f "$TEAM_PIN" ] || die "$TEAM_PIN missing"
  local pin; pin="$(awk 'NF && !/^#/ { print $1; exit }' "$TEAM_PIN")"
  if g show-ref --verify --quiet refs/heads/main && [ "$(g rev-parse main)" = "$pin" ]; then
    info "main is your team's code ($pin)"; return
  fi
  [ -d "$TEAM_REPO/.git" ] || die "team repo not found: $TEAM_REPO (clone it, or set TEAM_REPO=/path/to/it)
         - or use base SWEB for everything: task.sh mode base"
  git -C "$TEAM_REPO" cat-file -e "$pin^{commit}" 2>/dev/null ||
    die "your team repo does not have commit $pin yet - update it first: git -C $TEAM_REPO fetch"
  info "importing your team's code ($pin) from $TEAM_REPO - read only, nothing is changed there"
  g fetch -q "$TEAM_REPO" "$pin"
  if [ "$(g symbolic-ref --quiet --short HEAD 2>/dev/null)" = main ]; then
    [ -z "$(g status --porcelain)" ] || die "uncommitted changes on main - commit or throw them away first"
    g switch -q -C main "$pin"
  else
    g branch -f main "$pin" >/dev/null
  fi
  info "branch main = $(g log --oneline -1 main)"
}

# Older training copies had main = base SWEB (+ team/main = team code). Rename once, nothing is lost:
# main, task/..., solution/..., example/..., base/..., backup/... -> base-sweb/..., then team/X -> X.
migrate_layout() {
  g show-ref --verify --quiet refs/heads/base-sweb/main && return 0
  local b
  local old; old="$(g for-each-ref --format='%(refname:short)' refs/heads/main refs/heads/task \
    refs/heads/solution refs/heads/example refs/heads/base refs/heads/backup)"
  local team; team="$(g for-each-ref --format='%(refname:short)' refs/heads/team)"
  info "one main from now on: main = your team's code, base SWEB under base-sweb/ (branches are only renamed)"
  for b in $old; do g branch -q -m "$b" "base-sweb/$b"; done
  for b in $team; do g branch -q -m "$b" "${b#team/}"; done
  g show-ref --verify --quiet refs/heads/base-sweb/main || g branch -q base-sweb/main f6fcb2ab
  [ -n "$old" ] && info "your old base-SWEB branches: git -C $REPO branch --list 'base-sweb/*'"
  return 0
}

cmd_setup() {
  if [ -d "$REPO/.git" ]; then info "training repo already there: $REPO"; return; fi
  local practice="$HOME/Documents/University/2026-fall/os/repos/sweb"
  mkdir -p "$(dirname "$REPO")"
  if [ -d "$practice/.git" ] && git -C "$practice" cat-file -e f6fcb2ab 2>/dev/null; then
    info "cloning upstream SWEB (f6fcb2ab) from your practice repo $practice"
    git clone -q "$practice" "$REPO"
  else
    info "cloning upstream SWEB from GitHub"
    git clone -q https://github.com/isec-tugraz/sweb.git "$REPO"
  fi
  git -C "$REPO" switch -q -C base-sweb/main f6fcb2ab
  git -C "$REPO" branch -q -D main 2>/dev/null || true
  git -C "$REPO" remote set-url origin https://github.com/isec-tugraz/sweb.git
  info "training repo ready: $REPO (base-sweb/main = upstream f6fcb2ab)"
  if [ -d "$TEAM_REPO/.git" ]; then cmd_team_setup; g switch -q main
  else info "team repo $TEAM_REPO not found - clone it, then: task.sh team setup"; fi
}

if [ "${1:-}" = setup ]; then cmd_setup; exit 0; fi
[ -d "$REPO/.git" ] || die "training repo $REPO not found - create it with: task.sh setup"
migrate_layout

# ---------------------------------------------------------------- base SWEB or team code
# team (default): everything starts from main (your team's code), patches from team/.
#   A task without a team version falls back to base SWEB (set_mode_for).
# base: everything starts from base-sweb/main (upstream SWEB), patches from solutions/ and examples/,
#   branches under base-sweb/.
# 'task.sh mode base|team' stores the choice in the training repo's git config. test and status work
# on the current branch, so for them the branch decides.
MODE="${TASK_MODE:-$(git -C "$REPO" config --get ag.mode 2>/dev/null || echo team)}"
if [[ "${1:-}" == test || "${1:-}" == status ]]; then
  case "$(git -C "$REPO" symbolic-ref --quiet --short HEAD 2>/dev/null)" in
    base-sweb/*) MODE=base ;;
    *) MODE=team ;;
  esac
fi
set_mode() {
  MODE="$1"
  case "$MODE" in
    team) BP=""; ROOT="main"; PATCHES="$COURSE/team" ;;
    base) BP="base-sweb/"; ROOT="base-sweb/main"; PATCHES="$COURSE" ;;
    *) die "unknown mode '$MODE' (team or base)" ;;
  esac
}
set_mode "$MODE"


cmd_mode() {
  case "${1:-}" in
    "") info "mode: $MODE (team = your team's code, the default; base = base SWEB)" ;;
    team)
      git -C "$REPO" config --unset ag.mode || true
      info "team mode (default): tasks, solutions and examples use your team's code (branch main)" ;;
    base)
      git -C "$REPO" config ag.mode base
      info "base mode: tasks, solutions and examples use base SWEB (branch base-sweb/main) - back: task.sh mode team" ;;
    *) die "task.sh mode team|base" ;;
  esac
}

case "${1:-}" in
  team) [ "${2:-}" = setup ] || die "task.sh team setup"; cmd_team_setup; exit 0 ;;
  mode) cmd_mode "${2:-}"; exit 0 ;;
esac

# team mode needs main = your team's code: import it on first use
pin_now="$(awk 'NF && !/^#/ { print $1; exit }' "$TEAM_PIN" 2>/dev/null || true)"
if [ "$MODE" = team ] && { ! g show-ref --verify --quiet refs/heads/main || [ "$(g rev-parse main)" != "$pin_now" ]; }; then
  case "${1:-}" in
    start|solution|diff|example|reset|list) cmd_team_setup ;;
  esac
fi

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
  if [ -n "$patch" ] && [ -s "$patch" ]; then     # an empty patch = nothing to add (a "study" task)
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

# patch_of ID DIR -> the solution/example patch for the current mode
patch_of() {
  local f
  if [[ "$1" == ex* ]]; then f="$PATCHES/$2/example.patch"; else f="$PATCHES/solutions/$2/solution.patch"; fi
  [ -f "$f" ] || die "no patch $f"
  echo "$f"
}

has_team_version() {   # has_team_version ID DIR
  if [[ "$1" == ex* ]]; then [ -f "$COURSE/team/$2/example.patch" ]; else [ -f "$COURSE/team/solutions/$2/solution.patch" ]; fi
}

# set_mode_for: in team mode, a task without a team version (A1, A2) uses base SWEB
set_mode_for() {
  if [ "$MODE" = team ] && ! has_team_version "$T_ID" "$T_DIR"; then
    set_mode base
    info "$T_ID has no version for your team's code yet - using base SWEB (branch base-sweb/main)" >&2
  fi
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
  info "mode: $MODE (tasks start from $ROOT$([ "$MODE" = team ] && echo " = your team's code"))"
  printf "%-6s %-30s %-9s %s\n" "id" "folder" "base" "state"
  local id dir base opts cmds
  while IFS='|' read -r id dir base opts cmds; do
    [[ -z "$id" || "$id" == \#* ]] && continue
    lookup "$id"
    local state=""
    local work; work="$(work_branch "$id" "$T_NAME")"
    if [ "$MODE" = team ] && ! has_team_version "$id" "$dir"; then state="(base SWEB)"; fi
    branch_exists "$work" && state="started"
    branch_exists "${BP}solution/$T_NAME" && state="${state:+$state, }solution branch"
    [ "$(current_branch)" = "$work" ] && state="$state  <- current"
    [ "$base" = main ] && base="$ROOT"
    printf "%-6s %-30s %-9s %s\n" "$id" "$dir" "$base" "$state"
  done < "$MANIFEST"
}

start_branch() {   # creates task/<name> from the base + tests, switches to it
  local ref; ref="$(base_ref "$T_BASE")"
  local c; c="$(commit_on "$ref" "start $T_NAME: test programs" "" $(tests_of "$T_DIR"))"
  g branch "${BP}task/$T_NAME" "$c"
  g switch -q "${BP}task/$T_NAME"
  info "on branch ${BP}task/$T_NAME (based on $ref)"
  info "task card: $COURSE/$T_DIR/README.md"
  info "test it:   task.sh test"
}

cmd_start() {
  lookup "$1"
  set_mode_for
  [[ "$T_ID" == ex* ]] && die "examples: task.sh example $T_ID"
  if branch_exists "${BP}task/$T_NAME"; then
    if ! g merge-base --is-ancestor "$ROOT" "${BP}task/$T_NAME"; then
      info "WARNING: ${BP}task/$T_NAME was started on an older version of $ROOT (your team code moved on)."
      info "         Start it again on the current code: task.sh reset $T_ID   (your attempt is kept as a backup)"
    fi
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
  local mode_saved="$MODE"
  local single=$(( ${#ids[@]} == 1 ))
  local id
  for id in "${ids[@]}"; do
    lookup "$id"
    set_mode "$mode_saved"; set_mode_for 2>/dev/null
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
  set_mode_for
  [[ "$T_ID" == ex* ]] && die "examples are complete already: task.sh example $T_ID"
  [ "$(current_branch)" = "${BP}solution/$T_NAME" ] || require_clean
  [ "$(current_branch)" = "${BP}solution/$T_NAME" ] && g switch -q "$ROOT"
  make_solution
  g switch -q "${BP}solution/$T_NAME"
  info "on branch ${BP}solution/$T_NAME - the reference solution, ready to build: task.sh test / task.sh run"
  info "notes: $COURSE/solutions/$T_DIR/NOTES.md"
  info "back to your attempt: task.sh start $T_ID"
}

cmd_diff() {
  lookup "$1"
  set_mode_for
  branch_exists "${BP}task/$T_NAME" || die "task $T_ID not started"
  make_solution
  g diff "${BP}task/$T_NAME" "${BP}solution/$T_NAME" -- . ':(exclude)userspace/tests'
}

cmd_example() {
  lookup "$1"
  set_mode_for
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
  b="${b#base-sweb/}"
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
  local cfg; cfg="$(g config --get ag.mode 2>/dev/null || echo team)"
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
  debug) build; echo y | make debug > /dev/null 2>&1 && make -j"$(nproc)" > make.log 2>&1 && make -j"$(nproc)" >> make.log 2>&1
         info "debug build - SWEB waits for the debugger. In a second terminal: task.sh gdb"; make qemugdb ;;
  gdb) cd "$BUILD" && make "run${2:-cgdb}" ;;
  nodebug) cd "$BUILD" && make x86_64 > /dev/null && info "normal (non-debug) build configured" ;;
  status) cmd_status ;;
  -h|--help|help|"") sed -n '2,29p' "$0" | sed 's/^# \{0,1\}//' ;;
  *) die "unknown command '$1' (task.sh help)" ;;
esac
