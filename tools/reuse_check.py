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

Three forms added 2026-11-11 (Recipe D, *A case tail, a returned local, a split
selection*):

  - a parameter `const T *p` passed `&t`, where `t` is filled by no added `t.m=e;`
    but by an initialiser `const T t = { &v1, &v2, ... };` (Recipe CT's targets): each
    entry must be an address `&v`, one per member of `typedef struct _T` in member
    order; the helper's binding of member `m` is read as a pointer parameter `m`
    bound to `&v` of its position (every `v` of the block `*m`); the initialiser's
    lines are allowed added lines when the step adds it;
  - a case tail: when the call of a `void` helper stands directly before `break;` in
    the working file, each `break;` the block lost is compared as `return;` (Recipe CT
    step 4; a `break;` of a loop nested in the block would be miscounted, which is
    checked by reading);
  - a returned local: `v=HELPER(...);` with `v` passed to no parameter, where HELPER
    declares a local `v` of its own (a type line and `v;`) and ends with `return(v);`:
    the declaration and the closing return are left out of the body compared (the
    block's `v` is the caller's `v`, assigned the helper's at the return; that the
    block writes `v` before reading it is checked by reading);
  - a split selection: when the call is the first statement of an added `default:`
    group and HELPER's body is one `switch` on the same expression as the `switch`
    that holds the call, HELPER's `switch (...)` line is left out of the body
    compared, and the added `default:` and the `break;` after the call are allowed
    (Recipe SD with an existing function as its new one).

It prints OK or FAIL with the line that broke the rule. Argument types against
parameter types, and the OpenMP condition of the step, are checked by reading.
"""
import glob
import os
import re
import subprocess
import sys
from collections import Counter

TRIVIAL = {"", "{", "}", "/*", "*/"}


def declared_return_type(func):
    """The return type of `func` as its header declares it, spaces removed (`char**`), or
    None. Reads MagickCore/*.h: the declaration is `extern MagickExport <base>` on one line
    and `  *...*func(...)` on a following indented line, or all on one line."""
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "MagickCore")
    for header in sorted(glob.glob(os.path.join(root, "*.h"))):
        lines = open(header, errors="replace").read().split("\n")
        for i, line in enumerate(lines):
            m = re.search(r"(\**)\s*\b" + re.escape(func) + r"\(", line)
            if not m or line.lstrip().startswith(("#", "/")):
                continue
            stars, head = m.group(1), line[:m.start()]
            if re.fullmatch(r"\s*", head):  # the declarator is on its own indented line
                j = i - 1
                while j >= 0 and (not lines[j] or lines[j][0] in " \t"):
                    j -= 1
                head = lines[j] if j >= 0 else ""
            head = re.sub(r"\b(extern|MagickExport|MagickPrivate)\b", "", head)
            if not re.fullmatch(r"[\w\s]+", head):
                continue  # a call or a cast inside an inline function, not a declaration
            return re.sub(r"\s+", "", head) + stars
    return None


def strip_identity_casts(line):
    """Recipe D, *Casts to a call's own return type, and a repeated (void)*: drop
    `(T) f(` where f's header declaration returns exactly T, and a `(void)` before
    another `(void)` cast."""
    line = re.sub(r"\(void\)\s*\(void\)", "(void)", line)

    def one(m):
        ret = declared_return_type(m.group(2))
        if ret is not None and ret == re.sub(r"\s+", "", m.group(1)):
            return m.group(2) + "("
        return m.group(0)
    return re.sub(r"\(([A-Za-z_][\w\s]*?\s*\**)\)\s*(\w+)\(", one, line)


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
    pat = re.compile(r"^(?:[\w\s]*?\**\s*(?:const\s+)?)?(\w+)\s*=\s*" + param
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
    ADDED_AT.clear()
    new_no = 0
    for line in diff.splitlines():
        h = re.match(r"@@ -\S+ \+(\d+)", line)
        if h:
            new_no = int(h.group(1))
            continue
        text = line[1:].strip()
        if line.startswith(("---", "+++")):
            continue
        if line[:1] == "+":
            new_no += 1
        if text in TRIVIAL:
            continue
        if line[:1] in "-+":
            (removed if line[0] == "-" else added).append(text)
            if line[0] == "+":
                ADDED_AT.append(new_no - 2)
    return removed, added


ADDED_AT = []  # 0-based working-file line of each added line, in order


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
    removed = [strip_identity_casts(r) for r in removed]
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
    call_at = added.index(call)
    call_parts = [call]
    k = call_at
    while call.count("(") > call.count(")") and k + 1 < len(added):
        k += 1  # a call wrapped over several lines: join them
        call_parts.append(added[k])
        call += added[k]
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
    targets_note = []
    allowed = set(call_parts)
    dropped = set()
    body_all = [b.strip() for b in body.splitlines()]
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
            if not fills:
                init = re.search(r"const\s+" + re.escape(stype) + r"\s+" + re.escape(local)
                                 + r"\s*=\s*\{([^{}]*)\};", text)
                tdef = re.search(r"typedef struct _" + re.escape(stype) + r"\s*\{(.*?)\}\s*"
                                 + re.escape(stype) + r";", text, re.S)
                if init and tdef:
                    members = re.findall(r"\**(\w+)\s*[,;]", tdef.group(1))
                    entries = split_args(init.group(1))
                    if len(entries) != len(members):
                        problems.append(f"initialiser of {local}: {len(entries)} entries "
                                        f"for {len(members)} members")
                    for member, entry in zip(members, entries):
                        if not re.fullmatch(r"&\w+", entry):
                            problems.append(f"initialiser entry {entry} is not &v")
                        if member in bound:
                            binds.append((entry, member, True))
                    for member in bound:
                        if member not in members:
                            problems.append(f"member {member} bound but not in {stype}")
                    for line in added:
                        if line == f"const {stype}" or (line.startswith(local + " = {")
                                                        and line.endswith("};")):
                            allowed.add(line)
                    targets_note.append(f"{local} initialised {{{init.group(1).strip()}}}")
                    continue
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
    flines = [l.strip() for l in text.split("\n")]
    at = [ADDED_AT[call_at]] if flines[ADDED_AT[call_at]] == call_parts[0] else []
    after = before = ""
    caller_switch = None
    if len(at) == 1:
        k = at[0] + len(call_parts)
        while k < len(flines) and flines[k] in ("", "}"):
            k += 1
        after = flines[k] if k < len(flines) else ""
        k = at[0] - 1
        while k >= 0 and flines[k] in ("", "{"):
            k -= 1
        before = flines[k]
        depth = 0
        for k in range(at[0] - 1, -1, -1):
            if flines[k] == "}":
                depth += 1
            elif flines[k] == "{":
                if depth == 0 and k > 0 and flines[k - 1].startswith("switch ("):
                    caller_switch = flines[k - 1]
                    break
                depth = max(depth - 1, 0)
    void_helper = re.search(r"^static void\b[^;{]*\b" + re.escape(helper) + r"\(",
                            text, re.M) is not None
    case_tail = void_helper and after == "break;" and "return;" in \
        [b.strip() for b in body.splitlines()]
    groups = before == "default:" and "default:" in added
    kept_break = False
    if groups:
        allowed.add("default:")
        i = added.index(call_parts[-1])
        if i + 1 < len(added) and added[i + 1] == "break;":
            allowed.add("break;")
        elif after == "break;":
            kept_break = True  # the diff kept a moved group's `break;` as context
    elif case_tail:
        i = added.index(call_parts[-1])
        if i + 1 < len(added) and added[i + 1] == "break;":
            allowed.add("break;")
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
        own_decl = [i for i in range(1, len(body_lines))
                    if body_lines[i] == f"{target};"
                    and re.fullmatch(r"[\w\s]+", body_lines[i - 1])]
        if not call.endswith(");") or not re.fullmatch(
                r"\w+=" + re.escape(helper) + r"\(.*\);", call):
            problems.append(f"the call is not the statement {target}={helper}(...);")
        elif not bound and len(own_decl) == 1 and body_lines \
                and body_lines[-1] == f"return({target});":
            d = own_decl[0]
            body_lines = body_lines[:d - 1] + body_lines[d + 1:-1]
            note += f", its own local {target} returned and assigned back to {target}"
        elif not bound:
            problems.append(f"{target} is assigned but passed to no parameter")
        elif not body_lines or body_lines[-1] != f"return({bound[0]});":
            problems.append(f"{helper} does not end with return({bound[0]});")
        else:
            body_lines = body_lines[:-1]
            note += f", return({bound[0]}) assigned back to {target}"
    elif re.match(r"^\w+\s*=", call):
        problems.append(f"the call assigns its result: {call}")
    if groups:
        head = body_lines[0] if body_lines else ""
        if caller_switch is None or not head.startswith("switch (") or \
                rewrite(caller_switch, binds) != head:
            problems.append(f"default: group, but {helper}'s body is not a switch on "
                            f"the caller's expression ({caller_switch} / {head})")
        else:
            body_lines = body_lines[1:]
            note += f", the groups of {caller_switch} after its new default:"
    rewritten = [rewrite(r, binds) for r in removed if not r.startswith("#")]
    if kept_break:
        rewritten.append("break;")
    if case_tail:
        rewritten = ["return;" if r == "break;" else r for r in rewritten]
        note += ", a case tail (break; compared as return;)"
    if targets_note:
        note += ", " + ", ".join(targets_note)
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
