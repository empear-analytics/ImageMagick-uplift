#!/usr/bin/env python3
"""Check the struct fill and the member bindings of a Recipe N or B step.

  struct_fill_check.py FILE TYPE LOCAL HELPER

TYPE is the typedef the step added, LOCAL the caller's local of that type, HELPER the
function that takes `const TYPE *`. The script checks, for the form Recipe N gives
(every member named after the caller's variable it carries):

  - the fill: either `LOCAL.m=m;` for every member, in declaration order, with
    nothing between the assignments and the call of HELPER that passes `&LOCAL`, or a
    `const TYPE LOCAL = { m1, m2, ... };` initialiser naming every member in order;
  - the bindings: every `x = p->m` in HELPER, where `p` is its `const TYPE *`
    parameter, binds a local named after the member it reads (`x` is `m`);
  - the move: every line the working tree removed from FILE against HEAD (`git diff`)
    reappears, indentation aside, among the lines it added.

It prints OK or FAIL with the line that broke the rule. It cannot check types or what
the block does with the members: those stay checks by reading (playbook, Recipe N
step 8 and *Unreached functions: B and N*).
"""
import re
import subprocess
import sys
from collections import namedtuple

TRIVIAL = {"", "{", "}", "/*", "*/"}
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
        if values != names:
            return [f"initialiser {values} is not {names}"]
        return []
    fill = "".join(r"\s*" + re.escape(f"{local}.{n}={n};") for n in names)
    call = r"\s*[^;\n]*\b" + re.escape(helper) + r"\([^;]*&" + re.escape(local) + r"\b"
    found = re.findall(fill + call, text)
    if len(found) != 1:
        return [f"expected one run of {local}.m=m; for {names} directly before "
                f"{helper}(...&{local}...), found {len(found)}"]
    stray = re.findall(re.escape(local) + r"\.(\w+)=(\w+);", text)
    return [f"{local}.{m}={v};" for m, v in stray if m != v]


def check_bindings(params, body, type_name, names):
    p = re.search(r"const " + re.escape(type_name) + r"\s*\*\s*(\w+)", params)
    if not p:
        return [f"no const {type_name} * parameter"]
    ptr = p.group(1)
    bad = []
    for m in re.finditer(r"\**\s*(\w+)\s*=\s*" + re.escape(ptr) + r"->(\w+)", body):
        if m.group(1) != m.group(2) or m.group(2) not in names:
            bad.append(m.group(0))
    return bad


def unmoved_lines(path):
    """Lines the working tree removed from FILE (against HEAD) that it added nowhere.

    A step of Recipe N or B moves text; it deletes nothing. Blank lines, braces and
    comment delimiters are ignored, and indentation does not count, nor the `,` or
    `;` that ends a declarator."""
    diff = subprocess.run(["git", "diff", "-U0", "HEAD", "--", path],
                          capture_output=True, text=True, check=True).stdout
    removed, added = [], []
    for line in diff.splitlines():
        text = line[1:].strip()
        if line.startswith(("---", "+++")) or text in TRIVIAL:
            continue
        if line[:1] in "-+":
            (removed if line[0] == "-" else added).append(text)
    # a declarator list loses or gains its last name: `*head,` becomes `*head;`
    added += [a.rstrip(",;") for a in added]
    removed = [r if r in added else r.rstrip(",;") for r in removed]
    return [f"removed but not moved: {r}" for r in removed if r not in added]


def main(argv):
    if len(argv) != 5:
        raise SystemExit(__doc__)
    step = Step(*argv[1:])
    type_name = step.type_name
    text = open(step.path).read()
    names = members(text, type_name)
    params, body = helper_body(text, step.helper)
    problems = check_fill(text, step, names)
    problems += check_bindings(params, body, type_name, names)
    problems += unmoved_lines(step.path)
    for line in problems:
        print("FAIL:", line)
    if problems:
        return 1
    print(f"OK {type_name}: {len(names)} members ({', '.join(names)}), "
          f"fill and bindings by name")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
