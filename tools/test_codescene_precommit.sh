#!/bin/bash
# test_codescene_precommit.sh: planted cases for the CodeScene gate's private headers.
# A throw-away git repository gets a copy of tools/codescene_precommit.py (or of
# $PRECOMMIT, to run the cases against another version) and stages one planted change
# per case. Needs cs-mcp, as the gate itself does (CS_MCP_BINARY_PATH, CS_ACCESS_TOKEN).
set -u
here=$(cd "$(dirname "$0")" && pwd)
gate=${PRECOMMIT:-$here/codescene_precommit.py}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
fail=0
repo=$tmp/repo
mkdir -p "$repo/tools" "$repo/MagickCore"
cp "$gate" "$repo/tools/codescene_precommit.py"
cp "$here/cs_mcp_session.py" "$repo/tools/"
good='static inline int Good(int x)
{
  return(x+1);
}
'
bad='static int Bad(int a,int b,int c,int d,int e,int f)
{
  int r=0;
  if (a > 0)
  {
    if (b > 0)
    {
      if (c > 0)
      {
        if (d > 0)
        {
          if (e > 0)
            r=1;
          else if (f > 0)
            r=2;
          else
            r=3;
        }
        else if (f > 1)
          r=4;
      }
      else if (e > 1)
        r=5;
    }
    else if (d > 1)
      r=6;
  }
  else if (c > 1)
    r=7;
  else if (b > 1)
    r=8;
  return(r);
}
'
printf '%s' "$good" > "$repo/MagickCore/x-private.h"
printf '%s' "$good" > "$repo/MagickCore/x.h"
printf '%s\n%s' "$good" "$bad" > "$repo/MagickCore/y.c"
printf '%s\n%s' "$good" "$bad" > "$repo/MagickCore/z-private.h"
git -C "$repo" init -q && git -C "$repo" add -A &&
  git -C "$repo" -c user.name=t -c user.email=t@t commit -qm base || exit 2

expect() {  # expect LABEL RC PATTERN: run the gate on what is staged, then reset
  out=$(cd "$repo" && python3 tools/codescene_precommit.py 2>&1); rc=$?
  if [ "$rc" -eq "$2" ] && echo "$out" | grep -q -- "$3"; then
    echo "ok   $1: $(echo "$out" | grep -- "$3" | head -1 | sed 's/^ *//')"
  else
    echo "FAIL $1 (rc $rc): $(echo "$out" | tail -2 | tr '\n' ' ')"; fail=1
  fi
  git -C "$repo" reset -q --hard && git -C "$repo" clean -qfd
}
printf '%s\n%s' "$good" "$bad" > "$repo/MagickCore/x-private.h"; git -C "$repo" add -A
expect "private header degrades" 1 "x-private.h: Code Health degraded from 10.00"
printf '%s\n%s' "$good" "$bad" > "$repo/MagickCore/x.h"; git -C "$repo" add -A
expect "public header degrades (not gated: nothing staged to score)" 0 ""
# A header cs-mcp cannot score passes only when tools/noscore_headers.txt (as staged) lists it.
printf '#ifdef NEVER\n  r=1;\n#endif\n' > "$repo/MagickCore/frag-private.h"; git -C "$repo" add -A
expect "new header with no score, not listed" 1 "frag-private.h (not in tools/noscore_headers.txt)"
printf '#ifdef NEVER\n  r=1;\n#endif\n' > "$repo/MagickCore/frag-private.h"
printf 'MagickCore/frag-private.h  # a fragment, no function\n' > "$repo/tools/noscore_headers.txt"
git -C "$repo" add -A
expect "new header with no score, listed in the same commit" 0 "gate passed"
printf '%s' "$bad" > "$repo/MagickCore/n-private.h"; git -C "$repo" add -A
expect "new private header below 10.0" 1 "n-private.h: new code must score 10.0"
printf '%s' "$good" > "$repo/MagickCore/y.c"
printf '%s' "$bad" > "$repo/MagickCore/y-bad-private.h"; git -C "$repo" add -A
expect "header carved off a shrinking .c (split bar)" 0 "gate passed"
# A new .c file is not held to a shrinking header's low score (review of 4e40bdec0).
printf '%s' "$good" > "$repo/MagickCore/z-private.h"
printf '%s' "$bad" > "$repo/MagickCore/w.c"; git -C "$repo" add -A
expect "new .c beside a shrinking low header" 1 "w.c: new code must score 10.0"
# cs-mcp gives no score for a header it cannot parse, as for one with no function: a header
# with function text and no score is refused, not counted as 10.0 (review of 4e40bdec0).
printf '}}} @@ static ((( \n%s\n' "$bad" > "$repo/MagickCore/x-private.h"; git -C "$repo" add -A
expect "private header made unparseable" 1 "x-private.h: No Code Health score returned"
# Bypasses of 3a760b93d's "no function text" heuristic (review of 3a760b93d): a K&R definition
# and a macro between ')' and '{'. Neither header is listed, so neither may pass on NO_SCORE.
printf '%s' "$bad" | sed '1s/(.*)$/(a,b,c,d,e,f)\n  int a,b,c,d,e,f;/' > "$repo/MagickCore/x-private.h"
git -C "$repo" add -A
expect "a K&R definition" 1 "x-private.h"
printf '%s' "$bad" | sed '1s/)$/) MAGICK_ATTRIBUTE/' > "$repo/MagickCore/x-private.h"; git -C "$repo" add -A
expect "a macro between ) and {" 1 "x-private.h"
printf '#ifdef NEVER\n  if (r != 0)\n  {\n    r=1;\n  }\n#endif\n' > "$repo/MagickCore/frag2-private.h"
printf 'MagickCore/frag2-private.h  # a fragment, no function\n' > "$repo/tools/noscore_headers.txt"
git -C "$repo" add -A
expect "new fragment with an if block, listed" 0 "gate passed"
[ $fail -eq 0 ] && echo "test_codescene_precommit: all passed"
exit $fail
