#!/usr/bin/env python3
"""Check the shared struct fill of Recipe N step 8's siblings across one function.

  sibling_fill_check.py FILE TYPE LOCAL HELPER

The form `struct_fill_check.py` checks fills the struct directly before its one call.
Under step 8's siblings across one function the local is filled once and serves several
calls that follow. This script checks that form, for the call of HELPER:

  - one run of `LOCAL.m=m;` for every member of TYPE, in declaration order, in the
    function that holds the calls of HELPER passing `&LOCAL`, and no other assignment
    to a member of LOCAL there;
  - every such call after the run, and all of them in that one function;
  - from the run to the last such call, no write to a member's variable: `m=`,
    `m op=`, `m++`, `m--`, `++m`, `--m`, or `&m` not followed by `->`, `.` or `[`;
  - the bindings and the move, as `struct_fill_check.py` checks them.

It prints OK or FAIL with what broke the rule. Types, and what the block does with the
members, stay checks by reading (playbook, Recipe N steps 3 and 8).
"""
import re
import subprocess
import sys
from collections import Counter

from struct_fill_check import TRIVIAL, Step, check_bindings, helper_body, members


def enclosing_function(text, pos):
    """(start, end) of the function body around pos: the column-0 braces."""
    start = text.rfind("\n{\n", 0, pos)
    end = text.find("\n}\n", pos)
    return start + 1, end + 2


def sibling_calls(text, step):
    """(function text, offsets of the calls in it), or a problem string."""
    call = re.compile(r"\b" + re.escape(step.helper) + r"\([^;]*&" +
                      re.escape(step.local) + r"\b")
    offsets = [c.start() for c in call.finditer(text)]
    if not offsets:
        return f"no call {step.helper}(...&{step.local}...)"
    start, end = enclosing_function(text, offsets[0])
    if offsets[-1] >= end:
        return f"calls of {step.helper} with &{step.local} lie in more than one function"
    return text[start:end], [o - start for o in offsets]


def member_write(span, name):
    """The first write to the variable `name` in span, or None."""
    n = re.escape(name)
    write = re.search(r"(?<![\w.>])" + n + r"\s*(?:[-+*/%&|^]|<<|>>)?=(?!=)|"
                      r"(?<![\w.>])" + n + r"\s*(?:\+\+|--)|"
                      r"(?:\+\+|--)\s*" + n + r"\b|"
                      r"&\s*" + n + r"\b(?!\s*(?:->|\.|\[))", span)
    return write.group(0) if write else None


def span_writes(span, names):
    writes = [(n, member_write(span, n)) for n in names]
    return [f"{n} is written between the fill and a call: {w!r}" for n, w in writes if w]


def check_sibling_fill(text, step, names):
    found = sibling_calls(text, step)
    if isinstance(found, str):
        return [found]
    fn, offsets = found
    local = step.local
    fill = re.compile("".join(r"\s*" + re.escape(f"{local}.{n}={n};") for n in names))
    fills = list(fill.finditer(fn))
    if len(fills) != 1:
        return [f"expected one run of {local}.m=m; for {names} in the caller, "
                f"found {len(fills)}"]
    assigned = len(re.findall(re.escape(local) + r"\.\w+\s*=", fn))
    problems = [f"{local} is assigned outside its fill"] * (assigned != len(names))
    problems += [f"a call of {step.helper} comes before the fill"] * (
        offsets[0] < fills[0].end())
    return problems + span_writes(fn[fills[0].end():offsets[-1]], names)


def unmoved_lines(path):
    """Lines the working tree removed against HEAD that it did not add as often, counted
    as a multiset (indentation, blank lines, braces and comment delimiters aside)."""
    diff = subprocess.run(["git", "diff", "-U0", "HEAD", "--", path],
                          capture_output=True, text=True, check=True).stdout
    lines = [(d[0], d[1:].strip()) for d in diff.splitlines()
             if d[:1] in "-+" and not d.startswith(("---", "+++"))]
    removed = Counter(t for s, t in lines if s == "-" and t not in TRIVIAL)
    added = Counter(t for s, t in lines if s == "+")
    return [f"removed but not moved: {t}" for t in removed - added]


def main(argv):
    if len(argv) != 5:
        raise SystemExit(__doc__)
    step = Step(*argv[1:])
    text = open(step.path).read()
    names = members(text, step.type_name)
    params, body = helper_body(text, step.helper)
    problems = check_sibling_fill(text, step, names)
    problems += check_bindings(params, body, step.type_name, names)
    problems += unmoved_lines(step.path)
    for line in problems:
        print("FAIL:", line)
    if problems:
        return 1
    print(f"OK {step.type_name}: {len(names)} members ({', '.join(names)}), "
          f"one fill before every call of {step.helper}, bindings by name")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
