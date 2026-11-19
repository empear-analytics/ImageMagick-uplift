#!/bin/sh
# obj_sorted.sh FILE [OLD_REV [NEW_REV]]
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
# moved lines must all be constant stores to distinct slots: such stores
# commute with each other and with code that does not read those slots.
# Exits 0 for IDENTICAL and REORDERED-CONSTANT-STORES, 1 otherwise.
set -e
file=$1
old=${2:-HEAD}
new=${3:-}
root=$(git rev-parse --show-toplevel)
build=$root/build-oracle/cand
[ -n "${WIDE:-}" ] && build=$root/build-oracle/cand-wide
cd "$build"
cflags=$(sed -n 's/^CFLAGS = //p' Makefile)
cppflags=$(sed -n 's/^CPPFLAGS = //p' Makefile)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
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
  # one line per instruction: "<function> <instruction> [<relocations>]"
  objdump -dr --no-show-raw-insn "$tmp/$side.o" | awk '
    /^[0-9a-f]+ <.*>:$/ { if (line != "") print line; line = ""; fn = $2; next }
    /^ *[0-9a-f]+:\t?R_/ { sub(/^ *[0-9a-f]+:[ \t]*/, ""); line = line " | " $0; next }
    /^ *[0-9a-f]+:/ { if (line != "") print line; sub(/^ *[0-9a-f]+:[ \t]*/, "");
                      gsub(/[ \t]+/, " "); sub(/ *#.*$/, ""); line = fn " " $0; next }
    END { if (line != "") print line }' > "$tmp/$side.lines"
  sort "$tmp/$side.lines" > "$tmp/$side.sorted"
done
grep -q '' "$tmp/after.lines" || { echo "NOTHING COMPILED $file in $build (try WIDE=1)"; exit 2; }
if cmp -s "$tmp/before.o" "$tmp/after.o"; then
  echo "IDENTICAL $file"
  exit 0
fi
if ! cmp -s "$tmp/before.sorted" "$tmp/after.sorted"; then
  echo "DIFFERENT $file (the sorted instruction lines differ)"
  diff "$tmp/before.sorted" "$tmp/after.sorted" | head -20
  exit 1
fi
# the lines that moved: those diff reports, in either direction
diff "$tmp/before.lines" "$tmp/after.lines" | sed -n 's/^[<>] //p' | sort -u > "$tmp/moved"
if grep -v -E ' mov[bwlq]? \$(0x)?-?[0-9a-f]+,(-?0x[0-9a-f]+)?\(%(rsp|rbp)\)$' "$tmp/moved" > "$tmp/other"; then
  echo "DIFFERENT $file (moved lines other than constant stores to stack slots)"
  cat "$tmp/other" | head -20
  exit 1
fi
slots=$(sed -E 's/.*,//' "$tmp/moved" | sort | uniq -d)
if [ -n "$slots" ]; then
  echo "DIFFERENT $file (two moved stores name one stack slot: $slots)"
  exit 1
fi
echo "REORDERED-CONSTANT-STORES $file ($(wc -l < "$tmp/after.lines" | tr -d ' ') instruction lines; moved:)"
sed 's/^/  /' "$tmp/moved"
exit 0
