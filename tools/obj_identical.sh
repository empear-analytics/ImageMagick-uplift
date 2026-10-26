#!/bin/sh
# obj_identical.sh FILE [REV]
#
# Compile FILE as committed at REV (default HEAD) and as it is in the working tree,
# with the oracle build's own flags (build-oracle/cand) at -g0, and compare the two
# object files byte for byte. Prints IDENTICAL or DIFFERENT and exits 0 or 1.
#
# This is the proof for Recipe L (hoist function-local types): a type's name and
# the place it is declared do not reach the machine code, so a step that only moves
# or renames a type must leave the object file unchanged. __LINE__
# and __FILE__ are pinned and both copies share the file's basename, as in
# pp_identical.sh.
set -e
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
done
if cmp -s "$tmp/before.o" "$tmp/after.o"
then
  echo "IDENTICAL $file ($(wc -c < "$tmp/after.o" | tr -d ' ') bytes of object code)"
  exit 0
fi
echo "DIFFERENT $file"
otool -tv "$tmp/before.o" > "$tmp/before.s"
otool -tv "$tmp/after.o" > "$tmp/after.s"
diff "$tmp/before.s" "$tmp/after.s" | head -20
exit 1
