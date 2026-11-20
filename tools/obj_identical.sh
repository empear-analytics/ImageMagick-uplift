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
#
# HDRS=1 compiles the REV side against REV's own headers (see below): the proof
# for a step that changes a header its includers inline from.
set -e
file=$1
rev=${2:-HEAD}
root=$(git rev-parse --show-toplevel)
build=$root/build-oracle/cand
# WIDE=1: the wide candidate (X11 and OpenCL), the only build that compiles
# accelerate.c, animate.c, display.c, opencl.c, widget.c and xwindow.c.
[ -n "${WIDE:-}" ] && build=$root/build-oracle/cand-wide
# BUILD=<dir>: any other configured tree under build-oracle/, for code the oracle's HDRI build leaves
# out (ci-clang-min, ci-clang-dlg: CI's non-HDRI configurations, which compile `#if !defined(
# MAGICKCORE_HDRI_SUPPORT)` branches); EXTRA_CFLAGS="-O2 -w" adds optimisation to those trees' -O0 and drops their -Werror diagnostics.
[ -n "${BUILD:-}" ] && build=$root/build-oracle/$BUILD
cd "$build"
cflags=$(sed -n 's/^CFLAGS = //p' Makefile)
cppflags=$(sed -n 's/^CPPFLAGS = //p' Makefile)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
base=$(basename "$file")
mkdir "$tmp/before" "$tmp/after"
git -C "$root" show "$rev:$file" > "$tmp/before/$base"
cp "$root/$file" "$tmp/after/$base"
# HDRS=1: compile the REV side against REV's own headers. Without it both sides
# include the working tree's headers, so a step that changes only a header (or a
# header and the file) is invisible here. The .h files of MagickCore/, MagickWand/
# and FILE's directory as committed at REV are extracted into a temporary tree that
# the REV side searches before the working tree (after the build directory, whose
# generated headers both sides share). Headers new in the working tree are not in
# REV, so the REV side finds them in the working tree, as REV's code never names them.
if [ -n "${HDRS:-}" ]
then
  mkdir "$tmp/hdrs"
  git -C "$root" ls-tree -r --name-only "$rev" -- MagickCore MagickWand "$(dirname "$file")" |
    grep '\.h$' | sort -u > "$tmp/hdrs.list"
  (cd "$root" && xargs git archive "$rev" < "$tmp/hdrs.list") | tar -x -C "$tmp/hdrs"
  echo "HDRS=1: the $rev side compiles against $rev's headers ($(wc -l < "$tmp/hdrs.list" | tr -d ' ') files)" >&2
fi
for side in before after
do
  hb= hd=
  if [ "$side" = before ] && [ -n "${HDRS:-}" ]
  then
    hb="-I$tmp/hdrs" hd="-I$tmp/hdrs/$(dirname "$file")"
  fi
  ( cd "$tmp/$side" &&
    eval clang -c -Wno-builtin-macro-redefined -D__LINE__=0 \
      "-D__FILE__=\\\"pp\\\"" -DHAVE_CONFIG_H -I"$build" $hb -I"$root" \
      -I"$build/config" $hd -I"$root/$(dirname "$file")" $cppflags $cflags ${EXTRA_CFLAGS:-} -g0 \
      -o "$tmp/$side.o" "$base" ) || { echo "BUILD FAILED ($side)"; exit 1; }
done
# An object file with no function in it proves nothing: the build does not
# compile the file (all of it inside an #if this configuration leaves out).
objdump -d "$tmp/after.o" | grep -q '^[0-9a-f]* <[^>]*>:$' ||
  { echo "NOTHING COMPILED $file in $build (try WIDE=1)"; exit 2; }
if cmp -s "$tmp/before.o" "$tmp/after.o"
then
  echo "IDENTICAL $file ($(wc -c < "$tmp/after.o" | tr -d ' ') bytes of object code)"
  exit 0
fi
echo "DIFFERENT $file"
# otool on macOS; objdump elsewhere (otool is missing on Linux, and set -e
# would end the script there with the status of the missing command).
for side in before after
do
  if command -v otool > /dev/null
  then otool -tv "$tmp/$side.o" > "$tmp/$side.s"
  else objdump -d --no-show-raw-insn "$tmp/$side.o" > "$tmp/$side.s"
  fi
done
diff "$tmp/before.s" "$tmp/after.s" | head -20
exit 1
