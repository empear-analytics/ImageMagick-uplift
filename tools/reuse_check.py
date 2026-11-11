#!/usr/bin/env python3
"""Check that a block replaced by a call of an existing helper is that helper's body.

  reuse_check.py FILE HELPER

For Recipe D's *Unreached functions* form when the object file is not byte-identical
(playbook, *Two levels of evidence*). The working tree must differ from HEAD in FILE by
one replacement: lines removed from one place, and one added line calling HELPER, an
existing `static` function of FILE. The script

  - reads HELPER's parameters and the call's arguments, position by position;
  - rewrites the removed lines as the call binds them: an argument `&v` passed to a
    pointer parameter `p` makes every `v` of the block `(*p)` and every `&v` of it `p`
    (Recipe PS step 4, Recipe O step 5); an argument that is an identifier or an
    expression `e` makes every `e` of the block `p` (Recipe D's consistent renaming);
    a name after `.` or `->` is a field and stays;
  - requires the rewritten removed lines and HELPER's body lines to be the same
    multiset, indentation aside and `(*p)` counted as `*p` (a declaration the block
    alone used goes with it and must match the helper's own);
  - requires that nothing else was added, and that no removed line is a `#pragma` or a
    preprocessor line (no OpenMP pragma moves; an `#if` around the call would be a
    different step).

A helper that returns the block's single output (Recipe E step 3, `v=Helper(v,...)`)
is accepted when the call is the statement `v=HELPER(...);` with `v` a plain name
passed as one of the arguments, and HELPER's body ends with `return(p);` for the
parameter `p` that argument binds: that one line is the assignment back and is not
compared. When the lines differ only in where they are broken (a wrapped expression
broken at another token), the rewritten block and HELPER's body are compared as one
token stream, in order, whitespace removed, and the script says so.

It prints OK or FAIL with the line that broke the rule. Argument types against
parameter types, and the OpenMP condition of the step, are checked by reading.
"""
import re
import subprocess
import sys
from collections import Counter

TRIVIAL = {"", "{", "}", "/*", "*/"}


def split_args(text):
    """Split a call's argument text at top-level commas."""
    out, depth, cur = [], 0, ""
    for ch in text:
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
            continue
        depth += ch in "([{"
        depth -= ch in ")]}"
        cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def helper_def(text, helper):
    m = re.search(r"^static [^;{]*\b" + re.escape(helper) + r"\((.*?)\)\s*\n\{(.*?)\n\}",
                  text, re.S | re.M)
    if not m:
        raise SystemExit(f"FAIL: helper {helper} not found")
    params = []
    for p in split_args(" ".join(m.group(1).split())):
        name = re.findall(r"\w+", p)[-1]
        params.append((name, "*" in p))
    return params, m.group(2)


def diff_lines(path):
    diff = subprocess.run(["git", "diff", "-U0", "HEAD", "--", path],
                          capture_output=True, text=True, check=True).stdout
    removed, added = [], []
    for line in diff.splitlines():
        text = line[1:].strip()
        if line.startswith(("---", "+++")) or text in TRIVIAL:
            continue
        if line[:1] in "-+":
            (removed if line[0] == "-" else added).append(text)
    return removed, added


def ident(e):
    return re.escape(e) if not re.fullmatch(r"\w+", e) else r"\b" + e + r"\b"


def rewrite(line, binds):
    """Apply the call's bindings at once, through placeholders.

    A name of the block that is a parameter's name but not the argument bound to it
    (`image` in the block, `excerpt_image` passed for `image`) is the caller's own
    variable, which the helper cannot see: it is marked so that it matches nothing."""
    holders = {}
    for k, (arg, param, _) in enumerate(binds):
        if arg not in (param, "&" + param):
            h = f"\1{k}\1"
            holders[h] = f"<caller's {param}>"
            line = re.sub(r"(?<![\w.>&])" + param + r"\b", h, line)
    for k, (arg, param, pointer) in enumerate(binds):
        if pointer and arg.startswith("&") and re.fullmatch(r"&\w+", arg):
            v = arg[1:]
            h1, h2 = f"\0{k}a\0", f"\0{k}b\0"
            holders[h1], holders[h2] = param, f"*{param}"
            line = re.sub(r"(?<![\w.>])&" + v + r"\b", h1, line)
            line = re.sub(r"(?<![\w.>\0])" + v + r"\b", h2, line)
        else:
            h = f"\0{k}c\0"
            holders[h] = param
            pat = ident(arg)
            if re.fullmatch(r"\w+", arg):
                pat = r"(?<![\w.>])" + pat
            line = re.sub(pat, h, line)
    for h, v in holders.items():
        line = line.replace(h, v)
    return canonical(line)


def canonical(line):
    return re.sub(r"\(\*(\w+)\)", r"*\1", line)


def main(argv):
    if len(argv) != 3:
        raise SystemExit(__doc__)
    path, helper = argv[1], argv[2]
    text = open(path).read()
    params, body = helper_def(text, helper)
    removed, added = diff_lines(path)
    calls = [a for a in added if re.search(r"\b" + re.escape(helper) + r"\(", a)]
    problems = []
    if len(calls) != 1:
        raise SystemExit(f"FAIL: expected one added call of {helper}, found {len(calls)}")
    call = calls[0]
    inner = call[call.index(helper + "(") + len(helper) + 1:]
    depth, end = 1, 0
    for end, ch in enumerate(inner):
        depth += ch == "("
        depth -= ch == ")"
        if depth == 0:
            break
    args = split_args(inner[:end])
    if len(args) != len(params):
        raise SystemExit(f"FAIL: {len(args)} arguments for {len(params)} parameters")
    binds = [(a, p, ptr) for a, (p, ptr) in zip(args, params)]
    for line in added:
        if line != call:
            problems.append(f"added besides the call: {line}")
    for line in removed:
        if line.startswith("#"):
            problems.append(f"preprocessor line removed: {line}")
    body_lines = [canonical(b.strip()) for b in body.splitlines()
                  if b.strip() not in TRIVIAL]
    returned = re.match(r"^(\w+)=" + re.escape(helper) + r"\(", call)
    note = ""
    if returned:
        target = returned.group(1)
        bound = [p for a, p, ptr in binds if a == target and not ptr]
        if not call.endswith(");") or not re.fullmatch(
                r"\w+=" + re.escape(helper) + r"\(.*\);", call):
            problems.append(f"the call is not the statement {target}={helper}(...);")
        elif not bound:
            problems.append(f"{target} is assigned but passed to no parameter")
        elif not body_lines or body_lines[-1] != f"return({bound[0]});":
            problems.append(f"{helper} does not end with return({bound[0]});")
        else:
            body_lines = body_lines[:-1]
            note = f", return({bound[0]}) assigned back to {target}"
    elif re.match(r"^\w+\s*=", call):
        problems.append(f"the call assigns its result: {call}")
    rewritten = [rewrite(r, binds) for r in removed if not r.startswith("#")]
    moved = Counter(rewritten)
    own = Counter(body_lines)
    if moved != own and "".join("".join(rewritten).split()) == \
            "".join("".join(body_lines).split()):
        note += " (the same token stream, broken into lines differently)"
    else:
        for line, n in (moved - own).items():
            problems.append(f"in the block, not in {helper} ({n}x): {line}")
        for line, n in (own - moved).items():
            problems.append(f"in {helper}, not in the block ({n}x): {line}")
    for line in problems:
        print("FAIL:", line)
    if problems:
        return 1
    print(f"OK {helper}: the block is its body under "
          + ", ".join(f"{a} -> {p}" for a, p, _ in binds) + note)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
