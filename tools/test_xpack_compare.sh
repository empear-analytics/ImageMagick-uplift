#!/bin/bash
# test_xpack_compare.sh: planted-defect test of tools/xpack_compare.sh check.
# The committed file against itself must print IDENTICAL; a copy with one colormap
# lookup changed in the first (LSB-first) and, separately, in the last (MSB-first)
# packer must print DIFFERENT, as must a copy whose MSB-first bitmap foreground bit moves.
# cover must pass a function every configuration runs and fail one the harness never calls.
set -uo pipefail
root=$(git rev-parse --show-toplevel)
cd "$root"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
git show HEAD:MagickCore/xwindow-image.c > "$tmp/orig.c"
fail=0
expect() {  # expect WORD FILE
  local got
  got=$(XPACK_CAND="$2" bash tools/xpack_compare.sh check HEAD 2>&1 | head -1 | cut -d: -f1)
  if [ "$got" = "$1" ]; then echo "ok   $1 $(basename "$2")"; else echo "FAIL expected $1, got $got ($(basename "$2"))"; fail=1; fi
}
expect IDENTICAL "$tmp/orig.c"
python3 - "$tmp" <<'EOF'
import sys
tmp = sys.argv[1]
s = open(tmp + '/orig.c', encoding='utf-8', errors='surrogateescape').read()
key = 'GetPixelIndex(canvas,p)]'
first, last = s.index(key), s.rindex(key)
assert first != last
for name, at in (('first', first), ('last', last)):
    t = s[:at] + 'GetPixelIndex(canvas,p)+1]' + s[at + len(key):]
    open('%s/planted-%s.c' % (tmp, name), 'w', encoding='utf-8', errors='surrogateescape').write(t)
key = '0x01 : 0x00);'
at = s.index(key, s.index('XMakeImageMSBFirst() initializes'))
at = s.index(key, at + 1)  # the foreground bit (the harness's foreground is the brighter)
t = s[:at] + '0x02 : 0x00);' + s[at + len(key):]
open(tmp + '/planted-bit.c', 'w', encoding='utf-8', errors='surrogateescape').write(t)
EOF
expect DIFFERENT "$tmp/planted-first.c"
expect DIFFERENT "$tmp/planted-last.c"
expect DIFFERENT "$tmp/planted-bit.c"
# cover: a packer every configuration runs reports no line left; XLogXImage, which the
# harness never calls, and a name the file lacks make it fail.
cover() {  # cover WANT_STATUS FUNCTION
  local out st
  out=$(XPACK_CAND="$tmp/orig.c" bash tools/xpack_compare.sh cover HEAD "$2" 2>&1); st=$?
  if [ "$st" = "$1" ]; then echo "ok   cover $2 exit $st"; else echo "FAIL cover $2 exit $st, expected $1: $out"; fail=1; fi
}
cover 0 XPackCanvasMSB
cover 1 XLogXImage
cover 1 NoSuchFunctionInTheFile
exit $fail
