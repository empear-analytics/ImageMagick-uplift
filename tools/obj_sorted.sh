#!/bin/sh
# obj_sorted.sh FILE [OLD_REV [NEW_REV]]
# obj_sorted.sh --self-test
#
# The check behind Recipe DI step 5's exception: compile FILE as committed at
# OLD_REV (default HEAD) and at NEW_REV (default: the working tree), with the
# oracle build's flags at -g0 (WIDE=1: the wide candidate, which compiles the
# OpenCL and X11 code), and compare the disassembly WITH RELOCATIONS
# (objdump -dr), function by function.
#
# Each instruction line keeps the relocation lines that follow it (a call's
# target, a data address), so a call to another function or a load of other
# data is a different line. Addresses and offsets are dropped. Then:
#
#   IDENTICAL                  the object files are byte for byte equal
#   REORDERED-CONSTANT-STORES  in every function the sorted lines are equal, and
#                              every line that moved is a store of an immediate
#                              constant to a stack slot (mov $imm,off(%rsp) or
#                              off(%rbp)), the moved stores naming distinct slots;
#                              the moved lines are printed for the commit message
#   DIFFERENT                  anything else (the differences are printed)
#
# Sorting alone would accept two calls that swapped places, which is why the
# moved lines must all be constant stores to distinct slots. The script does
# NOT check that no instruction between a moved store's old and new position
# reads that slot (a load, an lea, or a call taking its address): Recipe DI
# relies on its C-level argument for that.
# Exits 0 for IDENTICAL and REORDERED-CONSTANT-STORES, 1 otherwise.
#
# --self-test compiles small cases with plain clang -O2 and checks the verdicts:
# a retargeted call (a() -> b()) and two swapped calls (g(s); h(s) -> h(s); g(s))
# must be DIFFERENT; an unchanged file IDENTICAL.
set -e

# lines OBJ: one line per instruction, "<function> <instruction> [| <relocation>...]"
lines() {
  objdump -dr --no-show-raw-insn "$1" | awk '
    /^[0-9a-f]+ <.*>:$/ { if (line != "") print line; line = ""; fn = $2; next }
    /^[ \t]*[0-9a-f]+:[ \t]*R_/ { sub(/^[ \t]*[0-9a-f]+:[ \t]*/, ""); gsub(/[ \t]+/, " ");
                                  line = line " | " $0; next }
    /^[ \t]*[0-9a-f]+:/ { if (line != "") print line; sub(/^[ \t]*[0-9a-f]+:[ \t]*/, "");
                          gsub(/[ \t]+/, " "); sub(/ *#.*$/, ""); line = fn " " $0; next }
    END { if (line != "") print line }'
}

# compare DIR NAME: DIR holds before.o and after.o; prints the verdict, returns 0 or 1
compare() {
  d=$1; name=$2
  lines "$d/before.o" > "$d/before.lines"
  lines "$d/after.o" > "$d/after.lines"
  sort "$d/before.lines" > "$d/before.sorted"
  sort "$d/after.lines" > "$d/after.sorted"
  grep -q '' "$d/after.lines" || { echo "NOTHING COMPILED $name"; return 2; }
  if cmp -s "$d/before.o" "$d/after.o"; then
    echo "IDENTICAL $name"
    return 0
  fi
  if ! cmp -s "$d/before.sorted" "$d/after.sorted"; then
    echo "DIFFERENT $name (the sorted instruction lines differ)"
    diff "$d/before.sorted" "$d/after.sorted" | head -20
    return 1
  fi
  # the lines that moved: those diff reports, in either direction
  diff "$d/before.lines" "$d/after.lines" | sed -n 's/^[<>] //p' | sort -u > "$d/moved" || true
  if grep -v -E ' mov[bwlq]? \$(0x)?-?[0-9a-f]+,(-?0x[0-9a-f]+)?\(%(rsp|rbp)\)$' "$d/moved" > "$d/other"; then
    echo "DIFFERENT $name (moved lines other than constant stores to stack slots)"
    head -20 "$d/other"
    return 1
  fi
  slots=$(sed -E 's/.*,//' "$d/moved" | sort | uniq -d)
  if [ -n "$slots" ]; then
    echo "DIFFERENT $name (two moved stores name one stack slot: $slots)"
    return 1
  fi
  echo "REORDERED-CONSTANT-STORES $name ($(wc -l < "$d/after.lines" | tr -d ' ') instruction lines; moved:)"
  sed 's/^/  /' "$d/moved"
  return 0
}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

if [ "${1:-}" = "--self-test" ]; then
  fail=0
  check() {  # check NAME EXPECTED BEFORE_SOURCE AFTER_SOURCE
    mkdir -p "$tmp/$1/b" "$tmp/$1/a"
    printf '%s\n' "$3" > "$tmp/$1/b/x.c"; printf '%s\n' "$4" > "$tmp/$1/a/x.c"
    (cd "$tmp/$1/b" && clang -O2 -c -o ../before.o x.c)
    (cd "$tmp/$1/a" && clang -O2 -c -o ../after.o x.c)
    got=$(compare "$tmp/$1" "$1" | head -1 | cut -d' ' -f1) || true
    if [ "$got" = "$2" ]; then echo "ok   $1: $got"; else echo "FAIL $1: expected $2, got $got"; fail=1; fi
  }
  check retarget DIFFERENT 'void a(void); void b(void); void f(void) { a(); }' \
    'void a(void); void b(void); void f(void) { b(); }'
  check swap DIFFERENT 'void g(int *); void h(int *); void f(int *s) { g(s); h(s); }' \
    'void g(int *); void h(int *); void f(int *s) { h(s); g(s); }'
  check same IDENTICAL 'void g(int *); void f(int *s) { g(s); }' \
    'void g(int *); void f(int *s) { g(s); }'
  exit $fail
fi

file=$1
old=${2:-HEAD}
new=${3:-}
root=$(git rev-parse --show-toplevel)
build=$root/build-oracle/cand
[ -n "${WIDE:-}" ] && build=$root/build-oracle/cand-wide
cd "$build"
cflags=$(sed -n 's/^CFLAGS = //p' Makefile)
cppflags=$(sed -n 's/^CPPFLAGS = //p' Makefile)
base=$(basename "$file")
mkdir "$tmp/before" "$tmp/after"
git -C "$root" show "$old:$file" > "$tmp/before/$base"
if [ -n "$new" ]; then
  git -C "$root" show "$new:$file" > "$tmp/after/$base"
else
  cp "$root/$file" "$tmp/after/$base"
fi
for side in before after
do
  ( cd "$tmp/$side" &&
    eval clang -c -Wno-builtin-macro-redefined -D__LINE__=0 \
      "-D__FILE__=\\\"pp\\\"" -DHAVE_CONFIG_H -I"$build" -I"$root" \
      -I"$build/config" -I"$root/$(dirname "$file")" $cppflags $cflags -g0 \
      -o "$tmp/$side.o" "$base" ) || { echo "BUILD FAILED ($side)"; exit 1; }
done
if compare "$tmp" "$file"; then exit 0; else exit $?; fi
