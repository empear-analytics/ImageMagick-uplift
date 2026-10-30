#!/bin/sh
# obj_same_code.sh FILE [REV]
#
# Compile FILE as committed at REV (default HEAD) and as it is in the working tree,
# as tools/obj_identical.sh does (the oracle build's flags, -g0, __LINE__ and __FILE__
# pinned), and compare what the two object files contain rather than how they lay it
# out. Prints SAME CODE or DIFFERENT and exits 0 or 1.
#
# This is the proof for Recipe F (hoist function-local static data): a static that
# moves from a function body to file scope keeps its contents and the code that reads
# it, but the compiler may emit the data, and the string literals, in another order,
# so the object files differ byte for byte. Checked:
#   - the machine code of every function (otool -tv, addresses and all), which an
#     unrelocated object file holds before any data address is filled in;
#   - the set of C strings (__TEXT,__cstring), sorted;
#   - the size of every section;
#   - the bytes of every other data section, sorted, so that a table moved within its
#     section compares equal but a changed entry does not.
set -e
# otool is macOS's. Without it every listing below comes out empty on both sides
# (the failure is inside a pipeline, which set -e does not see) and the two would
# compare equal: refuse instead of printing a SAME CODE that compared nothing.
command -v otool >/dev/null 2>&1 || {
  echo "obj_same_code.sh: needs otool (macOS); nothing compared" >&2; exit 2; }
file=$1
rev=${2:-HEAD}
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
  o=$tmp/$side.o
  otool -tv "$o" | tail -n +2 > "$tmp/$side.text"
  otool -v -s __TEXT __cstring "$o" | tail -n +2 | sed 's/^[0-9a-f]*[[:space:]]*//' \
    | sort > "$tmp/$side.cstring"
  otool -l "$o" | awk '$1 == "sectname" {s=$2} $1 == "segname" {g=$2} $1 == "size" {print g, s, $2}' \
    > "$tmp/$side.sizes"
  : > "$tmp/$side.data"
  for sec in $(awk '{print $1 "," $2}' "$tmp/$side.sizes")
  do
    seg=${sec%,*}; name=${sec#*,}
    case "$name" in __text|__cstring|__compact_unwind|__eh_frame) continue ;; esac
    otool -s "$seg" "$name" "$o" | tail -n +3 | cut -f2 | tr ' ' '\n' | grep . | sort \
      | sed "s/^/$seg,$name /" >> "$tmp/$side.data"
  done
done
fail=0
for part in text cstring sizes data
do
  if ! cmp -s "$tmp/before.$part" "$tmp/after.$part"
  then
    echo "DIFFERENT $file: $part"
    diff "$tmp/before.$part" "$tmp/after.$part" | head -10
    fail=1
  fi
done
[ $fail -eq 0 ] && echo "SAME CODE $file ($(wc -l < "$tmp/after.text" | tr -d ' ') instructions)"
exit $fail
