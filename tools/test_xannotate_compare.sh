#!/bin/bash
# test_xannotate_compare.sh: planted-defect test of tools/xannotate_compare.sh check.
# The committed file against itself must print IDENTICAL; copies with one defect each must
# print DIFFERENT: the annotate pen's red read from green, the annotate box's alpha
# written for the transparent stencil instead of the opaque one (seen only by the
# annotate-alpha configurations), the draw pen's red read from green, and the draw
# background's alpha choice swapped.
# cover must pass a function every configuration runs and fail one the harness never calls.
set -uo pipefail
root=$(git rev-parse --show-toplevel)
cd "$root"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
git show HEAD:MagickCore/xwindow-annotate.c > "$tmp/orig.c"
fail=0
expect() {  # expect WORD FILE
  local got
  got=$(XANNOTATE_CAND="$2" bash tools/xannotate_compare.sh check HEAD 2>&1 | head -1 | cut -d: -f1)
  if [ "$got" = "$1" ]; then echo "ok   $1 $(basename "$2")"; else echo "FAIL expected $1, got $got ($(basename "$2"))"; fail=1; fi
}
expect IDENTICAL "$tmp/orig.c"
python3 - "$tmp" <<'EOF'
import sys
tmp = sys.argv[1]
s = open(tmp + '/orig.c', encoding='utf-8', errors='surrogateescape').read()

def plant(name, key, new, last=False):
    at = s.rindex(key) if last else s.index(key)
    t = s[:at] + new + s[at + len(key):]
    open('%s/planted-%s.c' % (tmp, name), 'w', encoding='utf-8',
         errors='surrogateescape').write(t)

key = 'pixel->pen_color.red'
assert s.index(key) != s.rindex(key)
plant('annotate-pen', key, 'pixel->pen_color.green')
plant('draw-pen', key, 'pixel->pen_color.green', last=True)
plant('annotate-box-alpha', 'annotate_info->stencil == OpaqueStencil',
      'annotate_info->stencil == TransparentStencil')
plant('draw-background-alpha', 'OpaqueStencil ? TransparentAlpha : OpaqueAlpha),q);',
      'OpaqueStencil ? OpaqueAlpha : TransparentAlpha),q);')
EOF
for f in annotate-pen draw-pen annotate-box-alpha draw-background-alpha; do
  expect DIFFERENT "$tmp/planted-$f.c"
done
# cover: a function every configuration runs reports no line left; XQuarterTurns, which
# the harness never calls, and a name the file lacks make it fail.
cover() {  # cover WANT_STATUS FUNCTION
  local out st
  out=$(XANNOTATE_CAND="$tmp/orig.c" bash tools/xannotate_compare.sh cover HEAD "$2" 2>&1); st=$?
  if [ "$st" = "$1" ]; then echo "ok   cover $2 exit $st"; else echo "FAIL cover $2 exit $st, expected $1: $out"; fail=1; fi
}
cover 0 XSetDrawRow
cover 1 XQuarterTurns
cover 1 NoSuchFunctionInTheFile
exit $fail
