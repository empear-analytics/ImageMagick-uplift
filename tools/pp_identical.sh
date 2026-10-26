#!/bin/sh
# pp_identical.sh FILE [REV]
#
# Preprocess FILE as committed at REV (default HEAD) and as it is in the working
# tree, with the oracle build's own flags (build-oracle/cand), and compare the two
# translation units. Prints IDENTICAL or DIFFERENT (with a short diff) and exits
# 0 or 1.
#
# This is the proof for Recipe H (hoist function-local macros): moving a #define
# leaves the preprocessed text unchanged when no token between its old and new
# position names the macro. __LINE__ and __FILE__ are pinned, as in the
# machine-code comparison of VERIFICATION.md, because ImageMagick's exception
# macros embed them; both copies share the file's basename so assert's
# __FILE_NAME__ matches; blank lines and line markers are dropped.
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
  eval clang -E -P -Wno-builtin-macro-redefined -D__LINE__=0 \
    "-D__FILE__=\\\"pp\\\"" -DHAVE_CONFIG_H -I. -I"$root" -I"$build/config" \
    -I"$root/$(dirname "$file")" $cppflags $cflags "$tmp/$side/$base" 2>/dev/null |
    grep -v '^[[:space:]]*$' > "$tmp/$side.i"
done
if cmp -s "$tmp/before.i" "$tmp/after.i"
then
  echo "IDENTICAL $file ($(wc -l < "$tmp/after.i" | tr -d ' ') preprocessed lines)"
  exit 0
fi
echo "DIFFERENT $file"
diff "$tmp/before.i" "$tmp/after.i" | head -20
exit 1
