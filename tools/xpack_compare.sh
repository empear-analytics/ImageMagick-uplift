#!/bin/bash
# xpack_compare.sh check [REV]
# xpack_compare.sh time [REV] CONFIG [RUNS] [REPS]
# xpack_compare.sh cover REV FUNCTION...
#
# Builds tools/xpack_harness.c twice against the wide build (build-oracle/cand-wide, the one
# that compiles the X11 code): once including MagickCore/xwindow-image.c as committed at REV
# (default HEAD), once including the working tree's file. Both link the same library, so the
# only difference between the two programs is the included file.
#
#   check  runs every configuration of the harness (LSB-first and MSB-first byte orders,
#          bitmap, colormapped and continuous ZPixmaps of 2 to 32 bits per pixel, gamma on and
#          off, mutable and immutable windows, two image sizes) on both programs and prints
#          IDENTICAL (every packed byte and matte byte hashes the same) or the differing lines.
#   time   alternates the two programs RUNS times (default 9) on configuration CONFIG
#          (xpack_harness list names them), REPS packings each (default 20) of a 1600x1200
#          image, and prints both medians and their ratio (candidate/base).
#   cover  (xpack_compare.sh cover REV FUNCTION...) builds the harness once more with the
#          candidate file, with --coverage -O0 (read by llvm-cov gcov), runs every configuration of check, and
#          prints for each FUNCTION of the candidate file its executable lines and those
#          that no configuration ran (gcov's #####), with their line numbers. Exit status 1
#          when a named function is missing or has a line that did not run.
# XPACK_CAND=<file> takes the candidate from that file instead of the working tree
# (tools/test_xpack_compare.sh plants defects that way).
set -euo pipefail
mode=${1:?usage: xpack_compare.sh check|time [REV] [CONFIG RUNS REPS]}
rev=${2:-HEAD}
root=$(git rev-parse --show-toplevel)
B=$root/build-oracle/cand-wide
out=$root/build-oracle/xpack
mkdir -p "$out"
git -C "$root" show "$rev:MagickCore/xwindow-image.c" > "$out/base-xwindow-image.c"
cp "${XPACK_CAND:-$root/MagickCore/xwindow-image.c}" "$out/cand-xwindow-image.c.new"
mv "$out/cand-xwindow-image.c.new" "$out/cand-xwindow-image.c"
cd "$B"
CC=$(sed -n 's/^CC = //p' Makefile)
CFLAGS=$(sed -n 's/^CFLAGS = //p' Makefile | sed 's/-fpass-plugin=[^ ]*//')
CPPFLAGS=$(sed -n 's/^CPPFLAGS = //p' Makefile)
if [ "$mode" = cover ]; then
  shift 2
  [ $# -gt 0 ] || { echo "usage: xpack_compare.sh cover REV FUNCTION..."; exit 2; }
  cov=$out/cover
  rm -rf "$cov"; mkdir -p "$cov"
  $CC $CFLAGS -O0 --coverage $CPPFLAGS -w -DHAVE_CONFIG_H -I"$B" -I"$root" -I"$B/config" \
    -I"$root/MagickCore" "-DXWINDOW_SOURCE=\"$out/cand-xwindow-image.c\"" \
    -c "$root/tools/xpack_harness.c" -o "$cov/xpack_harness.o"
  ./libtool --silent --tag=CC --mode=link $CC $CFLAGS --coverage \
    $(sed -n 's/^LDFLAGS = //p' Makefile) -o "$cov/harness" "$cov/xpack_harness.o" \
    MagickCore/libMagickCore-7.*.la
  "$cov/harness" check > "$cov/check.txt"
  (cd "$cov" && llvm-cov gcov -o "$cov" "$cov/xpack_harness.o" > /dev/null 2>&1)
  g=$cov/cand-xwindow-image.c.gcov
  [ -f "$g" ] || { echo "no gcov output for the candidate file"; exit 2; }
  status=0
  for fn in "$@"; do
    awk -F: -v fn="$fn" '
      BEGIN { re = "^(static|MagickExport|MagickPrivate).*[ *]" fn "\\(";
              cont = "^  \\**" fn "\\(" }
      { src = $0; sub(/^[^:]*:[^:]*:/, "", src); c = $1; gsub(/ /, "", c); n = $2 + 0 }
      !inside && (src ~ re || (src ~ cont && head)) { inside = 1; found = 1 }
      { head = (src ~ /^static [^;]*$/ && src !~ /\)$/) }
      inside {
        if (c != "-") { e++; if (c ~ /^#####|^=====/) { u++; lines = lines " " n } }
        if (src ~ /^}/) inside = 0
      }
      END {
        if (!found) { printf "%s: not found\n", fn; exit 1 }
        printf "%s: %d executable lines, %d not run%s\n", fn, e, u, (u ? ":" lines : "")
        exit (u > 0)
      }' "$g" || status=1
  done
  exit $status
fi
for side in base cand; do
  $CC $CFLAGS $CPPFLAGS -w -DHAVE_CONFIG_H -I"$B" -I"$root" -I"$B/config" -I"$root/MagickCore" \
    "-DXWINDOW_SOURCE=\"$out/$side-xwindow-image.c\"" -c "$root/tools/xpack_harness.c" \
    -o "$out/$side.o"
  ./libtool --silent --tag=CC --mode=link $CC $CFLAGS $(sed -n 's/^LDFLAGS = //p' Makefile) \
    -o "$out/$side" "$out/$side.o" MagickCore/libMagickCore-7.*.la
done
case $mode in
  check)
    "$out/base" check > "$out/base.txt"
    "$out/cand" check > "$out/cand.txt"
    n=$(wc -l < "$out/base.txt")
    if cmp -s "$out/base.txt" "$out/cand.txt"; then
      echo "IDENTICAL: $n configuration runs (base $rev, candidate working tree)"
    else
      echo "DIFFERENT:"; diff "$out/base.txt" "$out/cand.txt" | head -40; exit 1
    fi ;;
  time)
    config=${3:?CONFIG}; runs=${4:-9}; reps=${5:-20}
    : > "$out/tb.txt"; : > "$out/tc.txt"
    for i in $(seq "$runs"); do
      "$out/base" time "$config" "$reps" | tail -1 >> "$out/tb.txt"
      "$out/cand" time "$config" "$reps" | tail -1 >> "$out/tc.txt"
    done
    mb=$(sort -g "$out/tb.txt" | awk '{a[NR]=$1} END {print a[int((NR+1)/2)]}')
    mc=$(sort -g "$out/tc.txt" | awk '{a[NR]=$1} END {print a[int((NR+1)/2)]}')
    awk -v b="$mb" -v c="$mc" -v n="$config" -v r="$runs" \
      'BEGIN {printf "%s: base median %.4f s, candidate median %.4f s, ratio %.3f (%d alternating runs)\n", n, b, c, c/b, r}' ;;
  *) echo "usage: xpack_compare.sh check|time [REV] [CONFIG RUNS REPS]"; exit 2 ;;
esac
