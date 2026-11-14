#!/bin/bash
# omp_compare.sh: compare two OpenMP builds of magick at one and at four threads.
#
#   tools/omp_compare.sh build SRC DIR     configure (HDRI Q16, gcc -O2, --enable-openmp)
#                                          and make SRC into DIR; SRC is a source tree
#                                          (the working tree) or a commit, which is
#                                          exported with git archive into DIR/src first
#   tools/omp_compare.sh run BASE CAND CASES
#                                          run every line of CASES (magick arguments,
#                                          one command per line; '#' lines skipped)
#                                          with BASE and CAND, each at OMP_NUM_THREADS=1
#                                          and =4, and compare exit status, stdout and
#                                          stderr of CAND with BASE's at the same thread
#                                          count. Timing lines (Elapsed time, User time,
#                                          Pixels per second) and the clock-time
#                                          properties of a built-in image (date:create,
#                                          date:modify, date:timestamp) are dropped
#                                          before the comparison: they differ from run
#                                          to run of one binary.
#
# The oracle's builds are configured --disable-openmp, so they cannot see a change
# in how a parallel region shares or privatises a variable. The playbook's OpenMP
# steps (Recipe E step 8, N step 1, the owner's ruling of 2026-11-12) ask for the
# cases that reach a moved region to be compared at OMP_NUM_THREADS=1 and =4 against
# the baseline; this is that comparison, kept in the repository so that a reviewer
# can run it. It prints one line per command and thread count that differs and a
# last line "omp_compare: N runs, D diverged"; it exits 1 when D > 0.
set -u
mode=${1:-}
case "$mode" in
  build)
    src=$2; dir=$3
    mkdir -p "$dir"
    if [ ! -d "$src" ]; then
      root=$(git rev-parse --show-toplevel)
      if [ ! -d "$dir/src" ]; then
        mkdir -p "$dir/src"
        (cd "$root" && git archive "$src") | tar -x -C "$dir/src"
      fi
      src=$dir/src
    fi
    src=$(cd "$src" && pwd)
    mkdir -p "$dir/build"
    cd "$dir/build" || exit 1
    if [ ! -f Makefile ]; then
      CC=gcc CFLAGS="-O2 -g" "$src/configure" --disable-shared --enable-static \
        --enable-openmp --without-perl --disable-docs --with-fftw --without-x \
        > configure.log 2>&1 || { tail -20 configure.log; exit 1; }
    fi
    make -j8 > make.log 2>&1 || { tail -30 make.log; echo "omp_compare: make failed"; exit 1; }
    grep -q -- "-fopenmp" Makefile || { echo "omp_compare: not an OpenMP build"; exit 1; }
    echo "omp_compare: built $dir/build/utilities/magick"
    ;;
  run)
    base=$2; cand=$3; cases=$4
    tmp=$(mktemp -d)
    trap 'rm -rf "$tmp"' EXIT
    runs=0; bad=0
    while IFS= read -r line || [ -n "$line" ]; do
      case "$line" in ''|'#'*) continue ;; esac
      for t in 1 4; do
        for side in base cand; do
          bin=$base; [ $side = cand ] && bin=$cand
          # shellcheck disable=SC2086
          (cd "$tmp" && OMP_NUM_THREADS=$t MAGICK_THREAD_LIMIT=$t eval "\"$bin\" $line" \
            > "$tmp/$side.out" 2> "$tmp/$side.err"; echo $? > "$tmp/$side.rc")
          for s in out err; do
            grep -v -E 'Elapsed time|User time|Pixels per second|^ *date:(create|modify|timestamp): ' "$tmp/$side.$s" \
              > "$tmp/$side.$s.f" || true
          done
        done
        runs=$((runs+1))
        if ! cmp -s "$tmp/base.rc" "$tmp/cand.rc" || ! cmp -s "$tmp/base.out.f" "$tmp/cand.out.f" \
           || ! cmp -s "$tmp/base.err.f" "$tmp/cand.err.f"; then
          bad=$((bad+1))
          echo "DIVERGED threads=$t: $line"
        fi
      done
    done < "$cases"
    echo "omp_compare: $runs runs, $bad diverged"
    [ $bad -eq 0 ]
    ;;
  *)
    sed -n '2,25p' "$0"; exit 2 ;;
esac
