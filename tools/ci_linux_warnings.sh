#!/bin/bash
# Compile source files as the Linux jobs of ImageMagick's own CI do, with the
# warnings that nearest match the Windows jobs' MSVC /WX errors added.
#
#   tools/ci_linux_warnings.sh <file.c>...
#   tools/ci_linux_warnings.sh --campaign-a     every campaign A file (CAMPAIGNS.md)
#
# .github/workflows/main.yml's "Build Linux" jobs configure with
# -Wall -Wextra -Werror (gcc: -Wno-builtin-declaration-mismatch; clang:
# -Wno-unused-function -Wno-incompatible-library-redeclaration) and
# --with-quantum-depth=16 --enable-hdri=no, at -O0, in a container with no
# delegate libraries. tools/ci_warnings.sh checks the macOS job's flags against
# the oracle's HDRI build; neither sees what these jobs see.
#
# Four configurations, each configured once under build-oracle/ci-<cc>-<conf>:
#   gcc and clang  x  min (no delegates, no X: the CI container) and
#                     dlg (the installed delegates and FFTW, as the Windows jobs
#                     compile fourier.c and compare.c's FFT code; no X)
# with main.yml's CFLAGS, -Werror left out so a file's diagnostics show at once,
# and these added for MSVC (C4456/C4457/C4459 hides a declaration; C2440/C2664
# const to non-const pointer):
#   gcc    -Wshadow -Wdiscarded-qualifiers
#   clang  -Wshadow -Wincompatible-pointer-types-discards-qualifiers
# plus a scan for MSVC's C4065 (a switch with a default but no case label),
# which no gcc or clang warning reports. What the four found in campaign A on
# 2026-11-10 (VERIFICATION.md, *CI warnings*): helpers taking `Quantum **` for a
# caller's `Quantum *magick_restrict q`, `const` dropped between helpers, a
# parameter kept only for a pragma's shared() clause, a body-local hiding an
# outer variable, and default-only switches left by Recipe SD.
#
# Not covered: MSVC's C4702 (unreachable code) and C4701 (see ci_warnings.sh's
# -Wconditional-uninitialized), and the Windows-only #if branches.
#
# Diagnostics listed in KNOWN (upstream code the campaign has not touched; CI
# never compiles it with -Wshadow) are not reported. Prints each diagnostic once
# with the configurations that gave it and exits non-zero if there is any.
# JOBS=n compiles n files at once (default: nproc).
set -uo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT="$ROOT/build-oracle"
JOBS=${JOBS:-$(nproc)}

# Upstream diagnostics, by file and message (line numbers move), as extended
# regular expressions. Empty: on 2026-11-10 every campaign A file was clean.
KNOWN=()

campaign_a() {  # the campaign A originals of CAMPAIGNS.md and the files split off them
  local o f
  local originals="accelerate animate display opencl widget xwindow attribute channel
    colormap colorspace compare composite decorate distort effect enhance feature fourier
    fx gem histogram layer magnify montage morphology paint prepress quantize resample
    resize segment shear statistic threshold transform vision visual-effects"
  for o in $originals; do
    for f in "$ROOT/MagickCore/$o.c" "$ROOT"/MagickCore/"$o"-*.c; do
      [ -f "$f" ] && echo "MagickCore/${f##*/}"
    done
  done | sort -u
}

if [ "${1:-}" = --campaign-a ]; then
  mapfile -t FILES < <(campaign_a)
else
  FILES=("$@")
fi
[ ${#FILES[@]} -gt 0 ] || { sed -n '2,30p' "$0" >&2; exit 2; }

MIN_FLAGS=(--without-x --without-bzlib --without-djvu --without-dmr --without-fontconfig
  --without-freetype --without-heic --without-jbig --without-jpeg --without-jxl
  --without-lcms --without-lqr --without-lzma --without-openexr --without-openjp2
  --without-pango --without-png --without-raqm --without-raw --without-tiff --without-webp
  --without-xml --without-zip --without-zlib --without-zstd)
DLG_FLAGS=(--without-x --with-fftw)

ci_cflags() {
  case $1 in
    gcc) echo "-Wall -Wextra -Werror -Wno-builtin-declaration-mismatch" ;;
    clang) echo "-Wall -Wextra -Werror -Wno-unused-function -Wno-incompatible-library-redeclaration" ;;
  esac
}
extra_flags() {
  case $1 in
    gcc) echo "-Wshadow -Wdiscarded-qualifiers" ;;
    clang) echo "-Wshadow -Wincompatible-pointer-types-discards-qualifiers" ;;
  esac
}

configure_tree() {  # <cc> <conf>
  local cc=$1 conf=$2 b="$OUT/ci-$1-$2" flags cxx=g++
  [ "$cc" = clang ] && cxx=clang++
  [ -f "$b/Makefile" ] && return 0
  flags=$(ci_cflags "$cc")
  mkdir -p "$b"
  local extra=("${MIN_FLAGS[@]}")
  [ "$conf" = dlg ] && extra=("${DLG_FLAGS[@]}")
  (cd "$b" && CC=$cc CXX=$cxx CFLAGS="$flags" CXXFLAGS="$flags" \
     "$ROOT/configure" --with-quantum-depth=16 --enable-hdri=no --without-perl \
     --prefix=/usr "${extra[@]}" > configure.log 2>&1) \
    || { echo "configure failed, see $b/configure.log" >&2; return 1; }
}

CONFS="gcc-min clang-min gcc-dlg clang-dlg"
for c in $CONFS; do configure_tree "${c%-*}" "${c#*-}" & done
wait
for c in $CONFS; do
  [ -f "$OUT/ci-$c/Makefile" ] || { echo "no $OUT/ci-$c/Makefile" >&2; exit 2; }
done

compile_one() {  # <conf> <file>: prints "<diagnostic>\t<conf>"
  local c=$1 f=$2 b="$OUT/ci-$1" cc
  cc=${c%-*}
  local DEFS CPPFLAGS CFLAGS CC
  CC=$(sed -n 's/^CC = //p' "$b/Makefile")
  DEFS=$(sed -n 's/^DEFS = //p' "$b/Makefile")
  CPPFLAGS=$(sed -n 's/^CPPFLAGS = //p' "$b/Makefile")
  CFLAGS=$(sed -n 's/^CFLAGS = //p' "$b/Makefile" | sed 's/ -Werror / /')
  # shellcheck disable=SC2086  # the Makefile's flags are word lists
  (cd "$ROOT" && $CC -c -o /dev/null $DEFS -I"$b" -I"$b/config" -I. $CPPFLAGS $CFLAGS \
      $(extra_flags "$cc") "$f" 2>&1) \
    | grep -E "^[^ ]+:[0-9]+:[0-9]+: (error|warning):" \
    | sed "s#^$ROOT/##; s#^\./##" | sed "s/\$/\t$c/"
}
export -f compile_one extra_flags
export OUT ROOT

# each compile writes a file of its own: parallel writers to one pipe interleave
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
export TMP
for c in $CONFS; do for f in "${FILES[@]}"; do printf '%s %s\n' "$c" "${f#"$ROOT"/}"; done; done \
  | xargs -P "$JOBS" -n 2 bash -c 'compile_one "$0" "$1" > "$TMP/$0.${1//\//_}"'
raw=$(cat "$TMP"/* 2>/dev/null)

# clang 18 reports __func__ and __PRETTY_FUNCTION__ (const char[N]) passed to a
# const char * parameter as discarding qualifiers, in every file; gcc and MSVC do not.
raw=$(printf '%s\n' "$raw" | grep -v "passing 'const char\[[0-9]*\]' to parameter of type 'char \*'")

# Only diagnostics in the files asked about (not in the headers they include).
pat=$(printf '%s\n' "${FILES[@]}" | sed "s#^$ROOT/##; s/\./\\\\./g; s/^/^/; s/\$/:/" | paste -sd'|')
raw=$(printf '%s\n' "$raw" | grep -E "$pat")

# MSVC C4065: a switch whose body has a default label and no case label.
scan=$(cd "$ROOT" && python3 - "${FILES[@]}" <<'EOF'
import re, sys
def strip(s):
    # blank comments, string and character literals, and preprocessor lines, keeping offsets
    out = list(s)
    for m in re.finditer(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'|^[ \t]*#[^\n]*(?:\\\n[^\n]*)*', s, re.S | re.M):
        for i in range(m.start(), m.end()):
            if out[i] != '\n':
                out[i] = ' '
    return ''.join(out)
def match(t, i, o, c):
    d = 0
    while i < len(t):
        if t[i] == o: d += 1
        elif t[i] == c:
            d -= 1
            if d == 0: return i
        i += 1
    return -1
for f in sys.argv[1:]:
    t = strip(open(f, errors='replace').read())
    for m in re.finditer(r'\bswitch\s*\(', t):
        p = match(t, m.end() - 1, '(', ')')
        b = re.compile(r'\s*\{').match(t, p + 1)
        if not b: continue
        e = match(t, b.end() - 1, '{', '}')
        body, cases, dflt, i = t[b.end():e], 0, 0, 0
        while i < len(body):   # skip nested switches, count this one's labels
            n = re.compile(r'\bswitch\s*\(|\bcase\b|\bdefault\s*:').search(body, i)
            if not n: break
            if n.group().startswith('switch'):
                q = match(body, n.end() - 1, '(', ')')
                r = re.compile(r'\s*\{').match(body, q + 1)
                i = match(body, r.end() - 1, '{', '}') + 1 if r else q + 1
                continue
            if n.group() == 'case': cases += 1
            else: dflt += 1
            i = n.end()
        if dflt and not cases:
            line = t.count('\n', 0, m.start()) + 1
            col = m.start() - t.rfind('\n', 0, m.start())
            print(f"{f}:{line}:{col}: warning: switch has a default label and no case label (MSVC C4065)\tscan")
EOF
)

all=$(printf '%s\n%s\n' "$raw" "$scan" | grep . )
for k in "${KNOWN[@]}"; do all=$(printf '%s\n' "$all" | grep -Ev "$k"); done
if [ -z "$all" ]; then
  echo "ci_linux_warnings: OK (${#FILES[@]} file(s), $CONFS, C4065 scan)"
  exit 0
fi
# one line per diagnostic, with the configurations that gave it
printf '%s\n' "$all" | awk -F'\t' '{ if (!($1 in c)) { o[++n]=$1; c[$1]=$2 } else if (index(c[$1], $2) == 0) c[$1]=c[$1] " " $2 }
  END { for (i=1; i<=n; i++) print o[i] "  [" c[o[i]] "]" }' | sort -t: -k1,1 -k2,2n
exit 1
