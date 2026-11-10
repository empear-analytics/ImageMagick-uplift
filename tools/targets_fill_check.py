#!/usr/bin/env python3
"""Check the targets fill, the bindings and the rewrite of a Recipe I step.

  targets_fill_check.py FILE TYPE LOCAL HELPER

TYPE is the targets typedef the step added (members are pointers to the caller's
written variables, each named after its variable), LOCAL the caller's local of that
type, HELPER the new function that takes `const TYPE *`. The script checks, for the form
Recipe I gives:

  - the fill: `LOCAL.m=(&m);` for every member, in declaration order, with nothing
    between the assignments and the one call of HELPER that passes `&LOCAL`, or a
    `const TYPE LOCAL = { &m1, &m2, ... };` initialiser naming every member in order.
    Inside a function that itself bound `m` from a targets member, `LOCAL.m=(m);`
    passes the same address on and stands for `(&m)`;
  - the bindings: every `x = p->m` in HELPER, where `p` is its `const TYPE *`
    parameter, binds a local named after the member it reads (`x` is `m`);
  - the rewrite (Recipe I steps 2 and 5): take every line the working tree removed from
    FILE against HEAD (`git diff`), write each member name `m` in it as step 5 does
    (`&m` becomes `m`, any other `m` becomes `*m`; a name after `.` or `->` is a field
    and stays); write every `(*m)`, there and in HELPER's body, as `*m`. Then the
    removed lines and HELPER's body lines must be the same multiset, indentation and
    a declarator's trailing `,` or `;` aside, except: the bindings and their type
    lines that open the body; the one `return(MagickTrue);` that ends it; and the
    exits. Each removed exit (`goto L;`, `return ...;`) must reappear unchanged
    among the added lines outside HELPER (the caller's exit, step 7 or 8), and HELPER
    must hold as many `return(MagickFalse);` as the block had exits;
  - nothing else is added: every added line outside HELPER's body belongs to the
    typedef, HELPER's signature, the declaration of LOCAL, its fill, the call, or is
    the exit matched above.

It prints OK or FAIL with the line that broke the rule. It cannot check types, nor the
precedence of `*m` against `->` and `[]` (a wrong one does not compile for the pointer
and struct members Recipe I carries, and is checked by reading), nor what the caller
does after the call (playbook, *Unreached functions: B, N and I*).
"""
import re
import subprocess
import sys
from collections import Counter, namedtuple

TRIVIAL = {"", "{", "}", "/*", "*/"}
EXIT = re.compile(r"^(goto \w+;|return\b.*;)$")
Step = namedtuple("Step", "path type_name local helper")


def members(text, type_name):
    """Member names of `typedef struct _X { ... } TYPE;`, in declaration order."""
    m = re.search(r"typedef struct _\w+\s*\{([^{}]*)\}\s*" + re.escape(type_name) +
                  r"\s*;", text)
    if not m:
        raise SystemExit(f"FAIL: typedef {type_name} not found")
    names = []
    for decl in m.group(1).split(";"):
        for part in decl.split(","):
            words = re.findall(r"\w+", part)
            if words:
                names.append(words[-1])
    return names


def helper_body(text, helper):
    m = re.search(r"^static [^;{]*\b" + re.escape(helper) + r"\((.*?)\)\s*\n\{(.*?)\n\}",
                  text, re.S | re.M)
    if not m:
        raise SystemExit(f"FAIL: helper {helper} not found")
    return m.group(1), m.group(2)


def check_fill(text, step, names):
    type_name, local, helper = step.type_name, step.local, step.helper
    init = re.search(r"const " + re.escape(type_name) + r"\s+" + re.escape(local) +
                     r"\s*=\s*\{([^}]*)\};", text)
    if init:
        values = [v.strip() for v in init.group(1).split(",")]
        want = ["&" + n for n in names]
        if values != want:
            return [f"initialiser {values} is not {want}"]
        return []
    fill = "".join(r"\s*" + re.escape(f"{local}.{n}=(") + r"&?" + re.escape(f"{n});")
                   for n in names)
    call = r"\s*[^;\n]*\b" + re.escape(helper) + r"\([^;]*&" + re.escape(local) + r"\b"
    found = re.findall(fill + call, text)
    if len(found) != 1:
        return [f"expected one run of {local}.m=(&m); for {names} directly before "
                f"{helper}(...&{local}...), found {len(found)}"]
    stray = re.findall(re.escape(local) + r"\.(\w+)=\(&?(\w+)\);", text)
    return [f"{local}.{m}=({v});" for m, v in stray if m != v]


def check_bindings(params, body, type_name, names):
    p = re.search(r"const " + re.escape(type_name) + r"\s*\*\s*(\w+)", params)
    if not p:
        return None, [f"no const {type_name} * parameter"]
    ptr = p.group(1)
    bad, bound = [], []
    for m in re.finditer(r"\**\s*(?:const\s+)?(\w+)\s*=\s*" + re.escape(ptr) +
                         r"->(\w+)", body):
        if m.group(1) != m.group(2) or m.group(2) not in names:
            bad.append(m.group(0))
        bound.append(m.group(2))
    if sorted(bound) != sorted(names):
        bad.append(f"bound {bound}, members {names}")
    return ptr, bad


def forward(line, names):
    """A removed line as Recipe I step 5 writes it in the helper."""
    for n in names:
        line = re.sub(r"(?<![\w.>])&" + n + r"\b", "\0" + n, line)
        line = re.sub(r"(?<![\w.>\0])" + n + r"\b", "*" + n, line)
        line = line.replace("\0" + n, n)
    return line


def canonical(line, names):
    for n in names:
        line = re.sub(r"\(\*" + n + r"\)", "*" + n, line)
    return line


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


def check_rewrite(path, body, ptr, names):
    """Compare as multisets: each removed line moves once, nothing is added twice.

    A declarator list may lose or gain its last name (`*head,` becomes `*head;`), so
    a trailing `,` or `;` of a line does not count."""
    removed, added = diff_lines(path)
    body_lines = [b.strip() for b in body.splitlines()]
    body_lines = [b for b in body_lines if b not in TRIVIAL]
    outside = Counter(added)
    outside.subtract(Counter(body_lines))
    binding = re.compile(r"^\**\s*(?:const\s+)?\w+\s*=\s*" + re.escape(ptr) + r"->\w+[,;]$")
    helper, returns_false = Counter(), 0
    in_bindings = True  # the binding declarations open the body (step 5)
    for b in body_lines:
        c = canonical(b, names)
        if in_bindings and binding.match(b):
            continue
        if in_bindings and re.fullmatch(r"[A-Za-z_]\w*", c):
            continue  # the type line of a binding declaration (`Cluster`)
        in_bindings = False
        if c == "return(MagickFalse);":
            returns_false += 1
            continue
        if c == "return(MagickTrue);":
            continue
        helper[c.rstrip(",;")] += 1
    problems, exits = [], 0
    trues = [b for b in body_lines if b == "return(MagickTrue);"]
    if len(trues) != 1 or body_lines[-1] != "return(MagickTrue);":
        problems.append("the helper must end with its one return(MagickTrue);")
    for r in removed:
        f = canonical(forward(r, names), names).rstrip(",;")
        if helper[f] > 0:
            helper[f] -= 1
            continue
        if EXIT.match(r) and outside[r] > 0:
            outside[r] -= 1
            exits += 1
            continue
        problems.append(f"removed but not moved: {r}  (as {f})")
    if exits != returns_false:
        problems.append(f"{exits} exit(s) left the block, {returns_false} "
                        f"return(MagickFalse); in the helper")
    problems += [f"in {ptr}'s helper but not moved ({n}x): {c}"
                 for c, n in helper.items() if n > 0]
    return problems, outside


def check_outside(text, step, outside):
    """Lines added outside HELPER's body: only the typedef, HELPER's signature, the
    caller's targets local, its fill and the call (with its exit, matched above)."""
    typedef = re.search(r"typedef struct _\w+\s*\{[^{}]*\}\s*" +
                        re.escape(step.type_name) + r"\s*;", text).group(0)
    sig = re.search(r"^static [^;{]*\b" + re.escape(step.helper) + r"\([^{]*\)\s*\n\{",
                    text, re.M).group(0)
    allowed = {l.strip() for l in (typedef + "\n" + sig).splitlines()}
    allowed |= {step.type_name, "const " + step.type_name, step.local + ";",
                step.local + ","}
    init = re.search(r"^\s*(" + re.escape(step.local) + r"\s*=\s*\{[^}]*\};)", text, re.M)
    if init:
        allowed.add(init.group(1).strip())  # the const form, checked by check_fill
    problems = []
    for line, n in outside.items():
        if n <= 0 or line in allowed:
            continue
        if re.fullmatch(re.escape(step.local) + r"\.\w+=\(&?\w+\);", line):
            continue  # the fill, checked by check_fill
        if re.search(r"\b" + re.escape(step.helper) + r"\(", line):
            continue  # the call
        problems.append(f"added outside {step.helper} ({n}x): {line}")
    return problems


def main(argv):
    if len(argv) != 5:
        raise SystemExit(__doc__)
    step = Step(*argv[1:])
    text = open(step.path).read()
    names = members(text, step.type_name)
    params, body = helper_body(text, step.helper)
    problems = check_fill(text, step, names)
    ptr, bad = check_bindings(params, body, step.type_name, names)
    problems += bad
    if ptr:
        moved, outside = check_rewrite(step.path, body, ptr, names)
        problems += moved + check_outside(text, step, outside)
    for line in problems:
        print("FAIL:", line)
    if problems:
        return 1
    print(f"OK {step.type_name}: {len(names)} members ({', '.join(names)}), "
          f"fill, bindings and rewrite by name")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
