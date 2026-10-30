#!/bin/bash
# Compile source files with the warnings ImageMagick's own CI turns into errors.
#
#   tools/ci_warnings.sh <file.c>...
#
# The oracle builds use neither -Werror nor these warnings, so a step can pass
# the oracle and still break .github/workflows/main.yml. The flags are those of
# its "Build MacOS arm64" job, the strictest clang job: above all
# -Wdeclaration-after-statement, which rejects a declaration after a statement
# in the same block. -Wconditional-uninitialized stands in for MSVC's C4701
# ("potentially uninitialized local variable"), an error in the Windows jobs.
#
# Uses the candidate build's configuration (tools/oracle/build.sh cand) for the
# include paths and defines; checks only, writes no object file. Prints the
# diagnostics and exits non-zero if any file has one. WIDE=1 uses the wide
# build's (build.sh wide) instead, which compiles the X11 and OpenCL code.
set -uo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
B="$ROOT/build-oracle/cand"
[ -n "${WIDE:-}" ] && B="$ROOT/build-oracle/wide"
[ -f "$B/Makefile" ] || { echo "no $B/Makefile: run tools/oracle/build.sh cand (or wide) first" >&2; exit 2; }
[ $# -gt 0 ] || { sed -n '2,15p' "$0" >&2; exit 2; }

# The job adds -Werror; here every diagnostic is reported, so all of a file's show at once.
CI_FLAGS=(-Wall -Wextra -Wdeclaration-after-statement
          -Wno-deprecated-declarations -Wno-incompatible-library-redeclaration
          -Wno-incompatible-pointer-types-discards-qualifiers -Wno-missing-declarations
          -Wno-unused-function -Wno-unused-parameter
          -Wconditional-uninitialized)
CC=$(sed -n 's/^CC = //p' "$B/Makefile")
DEFS=$(sed -n 's/^DEFS = //p' "$B/Makefile")
CPPFLAGS=$(sed -n 's/^CPPFLAGS = //p' "$B/Makefile")
CFLAGS=$(sed -n 's/^CFLAGS = //p' "$B/Makefile")

# Diagnostics in code the campaign has not touched, which the Windows jobs
# accept: fx.c's GetImgAttrToken (upstream later initialised iaStr, f03eacf47).
KNOWN="fx\.c:.*variable 'iaStr' may be uninitialized"

failed=0
for f in "$@"; do
  # shellcheck disable=SC2086  # the Makefile's flags are word lists
  out=$($CC -fsyntax-only $DEFS -I"$B" -I"$B/config" -I"$ROOT" $CPPFLAGS $CFLAGS \
        "${CI_FLAGS[@]}" "$ROOT/${f#"$ROOT"/}" 2>&1 | grep -E "(error|warning):")
  out=$(printf '%s\n' "$out" | grep -Ev "$KNOWN" | grep .)
  if [ -n "$out" ]; then
    printf '%s\n' "$out"
    failed=1
  fi
done
[ $failed = 0 ] && echo "ci_warnings: OK ($# file(s))"
exit $failed
