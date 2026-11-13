#!/bin/bash
# Compile MagickCore files as C++, the way the Windows jobs of ImageMagick's
# own CI build them (MSVC compiles the library's C sources as C++ with /WX),
# and report only the diagnostics the upstream base does not have.
#
#   tools/cxx_warnings.sh <file.c>...
#   tools/cxx_warnings.sh --all            every MagickCore/*.c
#
# Why: C and C++ disagree on a few things the campaign's recipes touch. In C a
# comparison has type int, in C++ it has type bool, and C++ will not convert
# bool to an enum: `return(a == b);` in a function returning MagickBooleanType
# compiles in C and is MSVC error C2440 (C2664 for an argument). That is what
# broke main's Windows jobs after Recipe DT (2026-11-13). The other MSVC /W4
# classes seen on campaign files are mapped to clang++ warnings:
#   C4456/C4457/C4459 hides a declaration   -Wshadow
#   C4702 unreachable code                  -Wunreachable-code
#   C4100 unreferenced parameter            -Wunused-parameter
#   unused variable (Linux -Werror jobs)    -Wunused-variable
# and C4065 (a switch with a default but no case label) is a text scan.
#
# Uses the candidate build's configuration (tools/oracle/build.sh cand) for
# include paths and defines. The baseline is the merge base with upstream/main,
# checked out by the oracle under build-oracle/src/<sha> (built if missing with
# `git worktree add`). A diagnostic counts as new when its file, warning class
# and quoted identifier occur more often in the candidate than in the base;
# line numbers are ignored. Exit status 1 when any file has a new diagnostic.
set -uo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
B="$ROOT/build-oracle/cand"
[ -f "$B/Makefile" ] || { echo "no $B/Makefile: run tools/oracle/build.sh cand first" >&2; exit 2; }
[ $# -gt 0 ] || { sed -n '2,25p' "$0" >&2; exit 2; }

BASE_SHA=$(git -C "$ROOT" merge-base HEAD upstream/main)
BASE_SRC="$ROOT/build-oracle/src/$BASE_SHA"
if [ ! -d "$BASE_SRC/MagickCore" ]; then
  git -C "$ROOT" worktree add --detach "$BASE_SRC" "$BASE_SHA" > /dev/null 2>&1 ||
    { echo "cannot check out $BASE_SHA" >&2; exit 2; }
fi

if [ "$1" = "--all" ]; then
  set -- $(cd "$ROOT" && ls MagickCore/*.c)
fi

DEFS=$(sed -n 's/^DEFS = //p' "$B/Makefile")
CPPFLAGS=$(sed -n 's/^CPPFLAGS = //p' "$B/Makefile")
MAGICK_CFLAGS=$(sed -n 's/^MAGICK_CFLAGS = //p' "$B/Makefile")
FLAGS="-x c++ -fsyntax-only"

diag() {  # $1 source tree, $2 file relative to it -> normalised diagnostics
  # shellcheck disable=SC2086
  (cd "$1" && eval clang++ $FLAGS -Wno-everything -Werror=return-type \
      -Wshadow -Wunreachable-code -Wunused-parameter -Wunused-variable \
      -I"$B" -I"$B/MagickCore" -I. $DEFS $CPPFLAGS $MAGICK_CFLAGS "$2" 2>&1) |
    grep -E "^$2:[0-9]+:[0-9]+: (error|warning):" |
    sed -E "s/^([^:]+):[0-9]+:[0-9]+: (error|warning): /\1: \2: /; s/ \[-W/ [-W/" |
    sed -E "s/(candidate|note).*//" | sort
  # C4065: `switch (...)` whose body has `default:` and no `case`.
  awk -v F="$2" '
    /switch[ \t]*\(/ { s=1; depth=0; seen=0; hascase=0; hasdef=0 }
    s { for (i=1;i<=length($0);i++){ c=substr($0,i,1);
          if(c=="{"){depth++; seen=1} else if(c=="}"){depth--} }
        if ($0 ~ /(^|[^a-zA-Z_])case[ \t]/) hascase=1
        if ($0 ~ /default[ \t]*:/) hasdef=1
        if (seen && depth==0) { if (hasdef && !hascase) print F": warning: C4065 switch with default and no case"; s=0 } }
  ' "$1/$2" | sort
}

status=0
for f in "$@"; do
  f=${f#"$ROOT/"}
  cand=$(diag "$ROOT" "$f")
  if [ -f "$BASE_SRC/$f" ]; then base=$(diag "$BASE_SRC" "$f"); else base=""; fi
  new=$(diff <(printf '%s\n' "$base" | sed '/^$/d') <(printf '%s\n' "$cand" | sed '/^$/d') | sed -n 's/^> //p')
  # magick_unreferenced(x) expands to (x) under MSVC only, so clang++ still
  # reports the parameter; the Windows jobs do not.
  for p in $(grep -o "magick_unreferenced([a-z_A-Z0-9]*)" "$ROOT/$f" | sed 's/.*(\(.*\))/\1/' | sort -u); do
    new=$(printf '%s\n' "$new" | grep -v "unused parameter '$p'")
  done
  if [ -n "$new" ]; then
    status=1
    echo "== $f: new diagnostics (lines from: clang++ the file to see them)"
    printf '%s\n' "$new"
  fi
done
[ $status = 0 ] && echo "OK: $# file(s), no C++/MSVC-class diagnostic beyond the upstream base"
exit $status
