#!/bin/bash
# test_obj_hdrs.sh: planted-defect tests for HDRS=1 of tools/obj_identical.sh and
# tools/obj_modulo.sh (compile the REV side against REV's own headers).
# A throw-away git repository holds MagickCore/t.c, the header it inlines from and a
# fake build-oracle/cand whose Makefile gives the flags. A constant changed only in
# the header must show as DIFFERENT with HDRS=1, and is invisible without it (the
# blind spot the mode closes); an unchanged tree stays IDENTICAL either way.
set -u
here=$(cd "$(dirname "$0")" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
fail=0
repo=$tmp/repo
mkdir -p "$repo/MagickCore" "$repo/build-oracle/cand/config"
printf 'CFLAGS = -O2\nCPPFLAGS = \n' > "$repo/build-oracle/cand/Makefile"
cat > "$repo/MagickCore/t-private.h" <<'EOF'
static inline int Scale(int x)
{
  return(x*3);
}
EOF
cat > "$repo/MagickCore/t.c" <<'EOF'
#include "MagickCore/t-private.h"
int Twice(int x)
{
  return(Scale(x)+Scale(x+1));
}
int Once(int x)
{
  return(Scale(x));
}
EOF
git -C "$repo" init -q && git -C "$repo" add -A &&
  git -C "$repo" -c user.name=t -c user.email=t@t commit -qm base || exit 2

expect() {  # expect LABEL PATTERN COMMAND...
  local label=$1 pat=$2; shift 2
  out=$(cd "$repo" && "$@" 2>&1)
  if echo "$out" | grep -q -- "$pat"; then
    echo "ok   $label: $(echo "$out" | grep -- "$pat" | head -1)"
  else
    echo "FAIL $label: expected '$pat', got: $(echo "$out" | tail -3)"; fail=1
  fi
}
expect "unchanged, default" "^IDENTICAL" sh "$here/obj_identical.sh" MagickCore/t.c
expect "unchanged, HDRS=1" "^IDENTICAL" env HDRS=1 sh "$here/obj_identical.sh" MagickCore/t.c
sed -i 's/x\*3/x*4/' "$repo/MagickCore/t-private.h"   # the planted header-only change
expect "header changed, default (blind)" "^IDENTICAL" sh "$here/obj_identical.sh" MagickCore/t.c
expect "header changed, HDRS=1" "^DIFFERENT" env HDRS=1 sh "$here/obj_identical.sh" MagickCore/t.c
expect "obj_modulo header changed, default (blind)" "^IDENTICAL" \
  sh "$here/obj_modulo.sh" MagickCore/t.c Twice Scale
expect "obj_modulo header changed, HDRS=1" "^  Once differs" \
  env HDRS=1 sh "$here/obj_modulo.sh" MagickCore/t.c Twice Scale
expect "obj_modulo header changed, HDRS=1 verdict" "^DIFFERENT" \
  env HDRS=1 sh "$here/obj_modulo.sh" MagickCore/t.c Twice Scale
git -C "$repo" checkout -q -- MagickCore/t-private.h
printf '#define UNUSED_NEW 1\n' > "$repo/MagickCore/u-private.h"   # a header new in the tree
sed -i '1a #include "MagickCore/u-private.h"' "$repo/MagickCore/t.c"
expect "new header included, HDRS=1" "^IDENTICAL" env HDRS=1 sh "$here/obj_identical.sh" MagickCore/t.c
[ $fail -eq 0 ] && echo "test_obj_hdrs: all passed"
exit $fail
