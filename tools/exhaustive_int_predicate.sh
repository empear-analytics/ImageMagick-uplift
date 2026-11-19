#!/bin/sh
# exhaustive_int_predicate.sh HEADER FUNCTION [REV]
#
# For a `static inline MagickBooleanType FUNCTION(int code)` (or `const int`) whose body
# reads nothing but its one int parameter and its own locals: extract the function's
# text as committed at REV (default HEAD) and as it is in the working tree, compile
# both into one harness (renamed), and compare their results for every one of the
# 2^32 int values. Prints EQUIVALENT (and the number of values) or DIFFERENT with the
# first differing value; exits 0 or 1.
#
# This is the check for a step on a predicate that no file calls, so that no object
# file holds its code and obj_identical.sh has nothing to compare (token-private.h's
# IsUTFValid, Recipe GC, 2026-11-19). The function may call nothing; the harness
# defines MagickBooleanType as MagickCore/magick-type.h does (MagickFalse = 0,
# MagickTrue = 1).
set -e
header=$1; fn=$2; rev=${3:-HEAD}
root=$(git rev-parse --show-toplevel)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
git -C "$root" show "$rev:$header" > "$tmp/old.h"
cp "$root/$header" "$tmp/new.h"
python3 - "$tmp" "$fn" <<'EOF'
import re, sys
tmp, fn = sys.argv[1], sys.argv[2]
def grab(path, newname):
    t = open(path).read()
    m = re.search(r"static inline MagickBooleanType %s\((const )?int code\)\n\{.*?\n\}\n" % fn, t, re.S)
    if not m:
        sys.exit("function %s not found in %s" % (fn, path))
    return m.group(0).replace(fn + "(", newname + "(", 1)
src = ["#include <stdio.h>", "#include <stdint.h>",
       "typedef enum { MagickFalse = 0, MagickTrue = 1 } MagickBooleanType;",
       grab(tmp + "/old.h", "OldPredicate"), grab(tmp + "/new.h", "NewPredicate"),
       """int main(void)
{
  int64_t v;
  for (v=INT32_MIN; v <= INT32_MAX; v++)
    if (OldPredicate((int) v) != NewPredicate((int) v))
      {
        printf("DIFFERENT at %lld\\n",(long long) v);
        return(1);
      }
  printf("EQUIVALENT (4294967296 values)\\n");
  return(0);
}"""]
open(tmp + "/h.c", "w").write("\n".join(src) + "\n")
EOF
# -O0: every value really goes through both functions (an optimiser may fold the loop).
cc -O0 -Wall -Wextra -o "$tmp/h" "$tmp/h.c"
"$tmp/h"
