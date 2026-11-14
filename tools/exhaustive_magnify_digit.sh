#!/bin/sh
# Compare XSelectMagnifyDigit as committed (HEAD) with the working tree for every 32-bit key symbol.
#
#   sh tools/exhaustive_magnify_digit.sh [FILE]
#
# FILE defaults to MagickCore/display-image.c. The old function (HEAD) and the new one (the working tree,
# with its typedef and table when Recipe KO moved them above it) are cut out of the source, compiled
# against the real X11 key symbol macros, and called with every key symbol 0 .. 0xffffffff and with
# 4,096 larger ones (the `(int)` cast keeps the low 32 bits), storing into a struct that holds only
# `magnify.data`. It prints the number of key symbols compared and how many differ, and exits non-zero
# when any differ. It exists because the oracle runs only part of the groups of this function (the keypad
# run runs in no case); it proves the table of Recipe KO equal to the `switch` for every input, not only
# for the reached ones. Needs clang and the X11 development headers (the wide build's).
set -eu
file=${1:-MagickCore/display-image.c}
root=$(git rev-parse --show-toplevel)
cd "$root"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
git show "HEAD:$file" > "$tmp/head.c"
python3 - "$tmp" "$file" <<'PYEOF'
import sys

tmp, path = sys.argv[1], sys.argv[2]
SIG = "static void XSelectMagnifyDigit(XWindows *windows,const KeySym key_symbol)\n{"


def cut(text, with_table):
    i = text.index(SIG)
    j = text.index("\n}\n", i) + 3
    k = text.rindex("typedef struct _MagnifyDigitKey", 0, i) if with_table else i
    return text[k:j]


head = open(tmp + "/head.c", errors="surrogateescape").read()
work = open(path, errors="surrogateescape").read()
pre = ("#include <stdio.h>\n#include <stddef.h>\n#include <X11/Xlib.h>\n#include <X11/keysym.h>\n"
       "typedef struct { struct { int data; } magnify; } XWindows;\n")
open(tmp + "/old.h", "w").write(pre + cut(head, False).replace("XSelectMagnifyDigit", "OldSelect") + "\n")
open(tmp + "/new.h", "w").write(cut(work, "typedef struct _MagnifyDigitKey" in work).replace(
    "XSelectMagnifyDigit", "NewSelect") + "\n")
PYEOF
cat > "$tmp/t.c" <<'CEOF'
#include "old.h"
#include "new.h"
int main(void)
{
  XWindows a, b;
  unsigned long k, bad = 0, n = 0;
  for (k = 0; k <= 0xfffffffful; k++)
  {
    a.magnify.data = -7; b.magnify.data = -7;
    OldSelect(&a, (KeySym) k);
    NewSelect(&b, (KeySym) k);
    n++;
    if (a.magnify.data != b.magnify.data) { bad++; if (bad < 5) printf("DIFF %lx %d %d\n", k, a.magnify.data, b.magnify.data); }
  }
  for (k = 0; k < 4096; k++)
  {
    unsigned long v = (k << 32) | (0x30 + (k % 70)) | ((k % 3) ? 0xffb0ul - 0x30 : 0);
    a.magnify.data = -7; b.magnify.data = -7;
    OldSelect(&a, (KeySym) v);
    NewSelect(&b, (KeySym) v);
    n++;
    if (a.magnify.data != b.magnify.data) { bad++; if (bad < 5) printf("DIFF %lx\n", v); }
  }
  printf("%lu key symbols compared, %lu differ\n", n, bad);
  return bad != 0;
}
CEOF
clang -O1 -I"$tmp" "$tmp/t.c" -o "$tmp/t"
"$tmp/t"
