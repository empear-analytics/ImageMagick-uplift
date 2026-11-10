#!/bin/sh
# obj_modulo.sh FILE CALLER HELPER [REV]
#
# The object-code check for Recipes B and N on a function no oracle case reaches
# (playbook, *Two levels of evidence*, *Unreached functions: B and N*).
#
# Compiles FILE as committed at REV (default HEAD) and as it is in the working tree,
# with the oracle build's own flags (build-oracle/cand) at -g0, __LINE__ and __FILE__
# pinned, as tools/obj_identical.sh does. Then:
#   - byte-identical object files: prints IDENTICAL and exits 0;
#   - otherwise disassembles both per function, with addresses and call/jump offsets
#     removed, and requires
#       (1) every function present on both sides, other than CALLER (the function the
#           step changed) and HELPER (the function it added, or for Recipe B the
#           function whose parameters it bundled), to be identical instruction for
#           instruction;
#       (2) the functions present after but not before to be HELPER alone;
#       (3) the functions present before but not after to be functions that HELPER's
#           body calls, or that CALLER's body calls and whose definition is the same
#           text on both sides (a different inlining decision, not a change of code);
#     and prints MODULO-IDENTICAL and exits 0, or DIFFERENT and exits 1.
# CALLER and HELPER themselves are not compared here: their text is checked by
# refactor_guard.py (literals and calls) and by tools/struct_fill_check.py.
set -e
file=$1
caller=$2
helper=$3
rev=${4:-HEAD}
[ -n "$file" ] && [ -n "$caller" ] && [ -n "$helper" ] || {
  echo "usage: obj_modulo.sh FILE CALLER HELPER [REV]"; exit 2; }
root=$(git rev-parse --show-toplevel)
build=$root/build-oracle/cand
cd "$build"
cflags=$(sed -n 's/^CFLAGS = //p' Makefile)
cppflags=$(sed -n 's/^CPPFLAGS = //p' Makefile)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
base=$(basename "$file")
mkdir "$tmp/before" "$tmp/after"
git -C "$root" show "$rev:$file" > "$tmp/before/$base"
cp "$root/$file" "$tmp/after/$base"
for side in before after
do
  ( cd "$tmp/$side" &&
    eval clang -c -Wno-builtin-macro-redefined -D__LINE__=0 \
      "-D__FILE__=\\\"pp\\\"" -DHAVE_CONFIG_H -I"$build" -I"$root" \
      -I"$build/config" -I"$root/$(dirname "$file")" $cppflags $cflags -g0 \
      -o "$tmp/$side.o" "$base" ) || { echo "BUILD FAILED ($side)"; exit 1; }
done
if cmp -s "$tmp/before.o" "$tmp/after.o"
then
  echo "IDENTICAL $file ($(wc -c < "$tmp/after.o" | tr -d ' ') bytes of object code)"
  exit 0
fi
for side in before after
do
  mkdir "$tmp/$side.fn"
  objdump -d --no-show-raw-insn "$tmp/$side.o" | awk -v dir="$tmp/$side.fn" '
    /^[0-9a-f]+ <[^>]+>:$/ { name=$2; gsub(/[<>:]/,"",name); out=dir "/" name; next }
    out != "" && /^ *[0-9a-f]+:\t/ {
      sub(/^ *[0-9a-f]+:\t/,"");
      gsub(/[0-9a-f]+ <[^>+]+\+0x[0-9a-f]+>/,"<local>");
      gsub(/[0-9a-f]+ <([^>]+)>/,"<&>");
      gsub(/<[0-9a-f]+ /,"<");
      gsub(/# [0-9a-f]+ <[^>]*>/,"");
      print > out }'
  ls "$tmp/$side.fn" | sort > "$tmp/$side.syms"
done
rc=0
only_after=$(comm -13 "$tmp/before.syms" "$tmp/after.syms" | tr '\n' ' ')
only_before=$(comm -23 "$tmp/before.syms" "$tmp/after.syms" | tr '\n' ' ')
echo "functions only after: ${only_after:-none}"
echo "functions only before: ${only_before:-none}"
for f in $only_after
do
  [ "$f" = "$helper" ] || { echo "  unexpected new function $f"; rc=1; }
done
fn_text() {  # fn_text NAME FILE: the definition of NAME in FILE
  awk -v h="$1" '
    $0 ~ "^(static|MagickExport|MagickPrivate|ModuleExport) .*[ *]" h "\\(" { on=1 }
    on { print }
    on && /^}/ { exit }' "$2"
}
body=$(fn_text "$helper" "$root/$file")
caller_body=$(fn_text "$caller" "$root/$file")
for f in $only_before
do
  echo "$body" | grep -q "[^A-Za-z0-9_]$f(" && continue
  # A function the shortened caller now inlines: allowed when the caller calls it
  # and its definition is the same text on both sides (an inlining decision, not a
  # change of code).
  if echo "$caller_body" | grep -q "[^A-Za-z0-9_]$f(" &&
     [ -n "$(fn_text "$f" "$tmp/before/$base")" ] &&
     [ "$(fn_text "$f" "$tmp/before/$base")" = "$(fn_text "$f" "$tmp/after/$base")" ]
  then
    echo "  $f now inlined into $caller (its text unchanged)"
    continue
  fi
  echo "  $f vanished but neither $helper calls it nor $caller with its text unchanged"
  rc=1
done
compared=0
for f in $(comm -12 "$tmp/before.syms" "$tmp/after.syms")
do
  [ "$f" = "$caller" ] && continue
  [ "$f" = "$helper" ] && continue
  compared=$((compared+1))
  cmp -s "$tmp/before.fn/$f" "$tmp/after.fn/$f" ||
    { echo "  $f differs"; diff "$tmp/before.fn/$f" "$tmp/after.fn/$f" | head -6; rc=1; }
done
echo "functions compared instruction for instruction: $compared"
if [ $rc -eq 0 ]
then
  echo "MODULO-IDENTICAL $file (all but $caller and $helper)"
else
  echo "DIFFERENT $file"
fi
exit $rc
