#!/bin/bash
# test_omp_compare.sh: planted-defect tests for tools/omp_compare.sh run.
# Fake magick binaries stand in for the OpenMP builds: the tool must report the same
# output as 0 diverged (timing lines ignored), and must report a difference that shows
# only at four threads, one only in stderr, and one only in the exit status.
set -u
here=$(cd "$(dirname "$0")" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
fail=0

mk() {  # mk NAME BODY: a fake magick
  printf '#!/bin/bash\n%s\n' "$2" > "$tmp/$1"; chmod +x "$tmp/$1"
}
mk base 'echo "args: $*"; echo "Elapsed time: $RANDOM"; echo "warn" >&2; exit 0'
mk same 'echo "args: $*"; echo "Elapsed time: $RANDOM$RANDOM"; echo "    date:create: $RANDOM"; echo "warn" >&2; exit 0'
mk date 'echo "args: $*"; echo "    date:created-by: $RANDOM"; echo "warn" >&2; exit 0'
mk race 'if [ "$OMP_NUM_THREADS" = 4 ]; then echo "args: $* x"; else echo "args: $*"; fi; echo "warn" >&2; exit 0'
mk errs 'echo "args: $*"; echo "other" >&2; exit 0'
mk code 'echo "args: $*"; echo "warn" >&2; exit 1'
printf '# a comment\nrose: -verbose info:\n\nlogo: -features 1 info:\n' > "$tmp/cases"

expect() {  # expect CAND STATUS PATTERN
  out=$(bash "$here/omp_compare.sh" run "$tmp/base" "$tmp/$1" "$tmp/cases"); rc=$?
  if [ $rc -ne "$2" ] || ! echo "$out" | grep -q -- "$3"; then
    echo "FAIL: $1 (rc $rc): $out"; fail=1
  else
    echo "ok: $1: $(echo "$out" | tail -1)"
  fi
}
expect same 0 "4 runs, 0 diverged"
expect date 1 "4 runs, 4 diverged"
expect race 1 "4 runs, 2 diverged"
expect race 1 "DIVERGED threads=4: rose: -verbose info:"
expect errs 1 "4 runs, 4 diverged"
expect code 1 "4 runs, 4 diverged"
[ $fail -eq 0 ] && echo "test_omp_compare: all passed"
exit $fail
