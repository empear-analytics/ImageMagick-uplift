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

Two forms added 2026-11-11 (Recipe D, *A helper defined further down or taking an
input struct*):

  - a forward declaration of HELPER may be added above the caller when the helper is
    defined below it: `static <type>` and `HELPER(<types>);`, whose parameter list
    must be the definition's with the parameter names left out;
  - a parameter `const S *p` passed `&s`, whose helper binds members at the top of its
    body (`T const m = p->m;`, a declarator `m = p->m,` of a multi-name declaration,
    each member bound once, under its own name): each binding is read as a parameter
    `m` and is bound to the expression `e` of the fill `s.m=e;` that the step added
    before the call; the fills, one per bound member and nothing else, and the
    declaration of `s` (its type line and `s;`) are allowed added lines, and the
    binding lines (with the type line that heads them) are left out of the body
    compared. An argument or fill `&E` or `(&E)` of a non-identifier `E` (a member,
    `&windows->widget`) also binds `E.` to `p->` (`(&E)->m` is `E.m`), and `&*p` is
    compared as `p` (the address of the object `p` points to).

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
    m = re.search(r"^static [^;{]*\b" + re.escape(helper) + r"\(([^;{]*?)\)\s*\n\{(.*?)\n\}",
                  text, re.S | re.M)
    if not m:
        raise SystemExit(f"FAIL: helper {helper} not found")
    params = []
    for p in split_args(" ".join(m.group(1).split())):
        name = re.findall(r"\w+", p)[-1]
        params.append((name, "*" in p, p))
    return params, m.group(2)


def prototype_types(params):
    """The definition's parameter list with the names left out, spaces removed."""
    out = []
    for _, _, text in params:
        out.append(re.sub(r"\w+$", "", text.strip()).replace(" ", ""))
    return out


def struct_bindings(body, param):
    """Members bound at the top of the body from `param->m` under their own name.

    Returns ({member: line index}, set of line indices to leave out: the bindings and
    the type lines that head them)."""
    lines = [b.strip() for b in body.splitlines()]
    bound, drop = {}, set()
    pat = re.compile(r"^(?:[\w\s]*?\*?\s*(?:const\s+)?)?(\w+)\s*=\s*" + param
                     + r"->(\w+)\s*[,;]$")
    for i, line in enumerate(lines):
        m = pat.match(line)
        if not m:
            continue
        if m.group(1) != m.group(2):
            raise SystemExit(f"FAIL: {line} binds a member under another name")
        if m.group(1) in bound:
            raise SystemExit(f"FAIL: member {m.group(1)} bound twice")
        bound[m.group(1)] = i
        drop.add(i)
        j = i - 1
        while j >= 0 and lines[j] and not re.search(r"[=;(){}]", lines[j]) \
                and j not in drop:
            drop.add(j)
            j -= 1
    return bound, drop, lines


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
            addr = re.fullmatch(r"\(?&([^()]+?)\)?", arg)
            if addr and not re.fullmatch(r"\w+", addr.group(1)):
                e = addr.group(1)
                hm = f"\0{k}m\0"
                holders[hm] = param + "->"
                line = re.sub(r"(?<![\w.>])" + re.escape(e) + r"\.", hm, line)
                line = re.sub(r"\(&" + re.escape(e) + r"\)", h, line)
                line = re.sub(r"(?<![\w.>])&" + re.escape(e) + r"\b", h, line)
                continue
            pat = ident(arg)
            if re.fullmatch(r"\w+", arg):
                pat = r"(?<![\w.>])" + pat
            line = re.sub(pat, h, line)
    for h, v in holders.items():
        line = line.replace(h, v)
    return canonical(line)


def canonical(line):
    line = re.sub(r"\(\*(\w+)\)", r"*\1", line)
    return re.sub(r"&\*(\w+)\b(?!\s*[\[.(]|->)", r"\1", line)


def main(argv):
    if len(argv) != 3:
        raise SystemExit(__doc__)
    path, helper = argv[1], argv[2]
    text = open(path).read()
    params, body = helper_def(text, helper)
    removed, added = diff_lines(path)
    proto_lines = set()
    for i, line in enumerate(added):
        if not (re.fullmatch(r"static [\w\s\*]+", line) and i + 1 < len(added)
                and added[i + 1].startswith(helper + "(")):
            continue
        j = i + 1
        while j < len(added) and not added[j].endswith(");") and "{" not in added[j]:
            j += 1
        if j < len(added) and added[j].endswith(");"):
            proto_lines |= set(added[i + 1:j + 1])
    calls = [a for a in added if re.search(r"\b" + re.escape(helper) + r"\(", a)
             and a not in proto_lines]
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
    binds = []
    allowed = {call}
    dropped = set()
    body_all = [b.strip() for b in body.splitlines()]
    call_at = added.index(call)
    for a, (p, ptr, ptext) in zip(args, params):
        s = re.fullmatch(r"&(\w+)", a)
        if ptr and s and re.match(r"const \w+ \*\s*\w+$", ptext.strip()) \
                and re.search(r"\b" + p + r"->\w+\s*[,;]", body):
            local, stype = s.group(1), ptext.split()[1]
            bound, drop, _ = struct_bindings(body, p)
            dropped |= drop
            fills = {}
            for i, line in enumerate(added):
                f = re.fullmatch(re.escape(local) + r"\.(\w+)=(.+);", line)
                if not f:
                    continue
                if i > call_at:
                    problems.append(f"fill after the call: {line}")
                if f.group(1) in fills:
                    problems.append(f"member {f.group(1)} filled twice")
                if f.group(1) not in bound:
                    problems.append(f"fill of a member {p} does not bind: {line}")
                fills[f.group(1)] = f.group(2)
                allowed.add(line)
            for member in bound:
                if member not in fills:
                    problems.append(f"member {member} bound but not filled")
                    continue
                binds.append((fills[member], member, False))
            allowed |= {stype, local + ";"}
            continue
        binds.append((a, p, ptr))
    joined = " ".join(l for l in added if l not in allowed)
    proto = re.search(r"static [\w\s\*]+?\b" + re.escape(helper) + r"\(([^()]*)\);",
                      joined)
    if proto:
        types = [t.replace(" ", "") for t in split_args(proto.group(1))]
        if types != prototype_types(params):
            problems.append(f"forward declaration {types} is not the definition's "
                            f"{prototype_types(params)}")
        for line in added:
            if line in proto.group(0) and line not in allowed:
                allowed.add(line)
        note_proto = ", forward declaration added"
    else:
        note_proto = ""
    for line in added:
        if line not in allowed:
            problems.append(f"added besides the call: {line}")
    for line in removed:
        if line.startswith("#"):
            problems.append(f"preprocessor line removed: {line}")
    body_lines = [canonical(b) for i, b in enumerate(body_all)
                  if b not in TRIVIAL and i not in dropped]
    returned = re.match(r"^(\w+)=" + re.escape(helper) + r"\(", call)
    note = note_proto
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
            note += f", return({bound[0]}) assigned back to {target}"
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
