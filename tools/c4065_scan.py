#!/usr/bin/env python3
"""Scan C files for MSVC's C4065: a switch whose body has a default label and no case label.

    python3 tools/c4065_scan.py FILE...     # one line per finding, as ci_linux_warnings.sh prints
    python3 tools/c4065_scan.py --self-test

No gcc or clang warning reports C4065, so tools/ci_linux_warnings.sh runs this scan.
Comments, string and character literals and preprocessor lines are blanked first,
keeping offsets. So are the lines of an `#if 0` branch (up to its `#else`, `#elif` or
`#endif`, nested conditionals counted): no compiler sees a case label there, MSVC
included, so a switch whose only case labels stand in an `#if 0` branch is reported
(morphology-builtin-base-private.h's IsBaseKernelNeeded keeps a real `case` for that
reason).
"""
import re
import sys

IF_ANY = re.compile(r"^[ \t]*#[ \t]*if")
IF_ZERO = re.compile(r"^[ \t]*#[ \t]*if[ \t]+0\b")
ELSE = re.compile(r"^[ \t]*#[ \t]*(else|elif)\b")
ENDIF = re.compile(r"^[ \t]*#[ \t]*endif\b")
NOISE = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\''
                   r"|^[ \t]*#[^\n]*(?:\\\n[^\n]*)*", re.S | re.M)
LABEL = re.compile(r"\bswitch\s*\(|\bcase\b|\bdefault\s*:")
BRACE = re.compile(r"\s*\{")


def next_depth(line, depth):
    """The depth inside an #if 0 branch after LINE (0: live code)."""
    if depth == 0:
        return 1 if IF_ZERO.match(line) else 0
    if IF_ANY.match(line):
        return depth + 1
    if ENDIF.match(line):
        return depth - 1
    if depth == 1 and ELSE.match(line):
        return 0
    return depth


def blank_dead_branches(s):
    """Blank every line of an #if 0 branch, its directive lines included."""
    lines = s.split("\n")
    depth = 0
    for k, line in enumerate(lines):
        new = next_depth(line, depth)
        if depth or new:
            lines[k] = " " * len(line)
        depth = new
    return "\n".join(lines)


def strip(s):
    """Blank comments, literals and preprocessor lines, keeping offsets."""
    out = list(blank_dead_branches(s))
    for m in NOISE.finditer("".join(out)):
        for i in range(m.start(), m.end()):
            if out[i] != "\n":
                out[i] = " "
    return "".join(out)


def match(t, i, o, c):
    """Index of the bracket C closing the bracket O at T[I], or -1."""
    d = 0
    for k in range(i, len(t)):
        d += (t[k] == o) - (t[k] == c)
        if d == 0 and t[k] == c:
            return k
    return -1


def skip_switch(body, n):
    """Index after a nested switch starting at match N."""
    q = match(body, n.end() - 1, "(", ")")
    r = BRACE.match(body, q + 1)
    return match(body, r.end() - 1, "{", "}") + 1 if r else q + 1


def count_labels(body):
    """(case labels, default labels) of this switch body, nested switches skipped."""
    cases = dflt = i = 0
    n = LABEL.search(body, i)
    while n:
        if n.group().startswith("switch"):
            i = skip_switch(body, n)
        else:
            cases += n.group() == "case"
            dflt += n.group() != "case"
            i = n.end()
        n = LABEL.search(body, i)
    return cases, dflt


def scan_text(name, text):
    """Findings in one file's text, as lines."""
    t = strip(text)
    found = []
    for m in re.finditer(r"\bswitch\s*\(", t):
        p = match(t, m.end() - 1, "(", ")")
        b = BRACE.match(t, p + 1)
        if not b:
            continue
        cases, dflt = count_labels(t[b.end():match(t, b.end() - 1, "{", "}")])
        if dflt and not cases:
            line = t.count("\n", 0, m.start()) + 1
            col = m.start() - t.rfind("\n", 0, m.start())
            found.append(f"{name}:{line}:{col}: warning: switch has a default label "
                         "and no case label (MSVC C4065)\tscan")
    return found


SELF_TESTS = [  # (name, source, findings expected)
    ("only #if 0 cases", "int f(int x){switch(x){\n#if 0\ncase 1:\n#else\ndefault:\n#endif\n"
     "return 1;}return 0;}\n", 1),
    ("a real case before #if 0", "int f(int x){switch(x){case 0:\n#if 0\ncase 1:\n#else\n"
     "default:\n#endif\nreturn 1;}return 0;}\n", 0),
    ("nested #ifdef inside #if 0", "int f(int x){switch(x){\n#if 0\n#ifdef A\ncase 1:\n#endif\n"
     "case 2:\n#endif\ndefault: return 1;}return 0;}\n", 1),
    ("case in the #else of #if 0", "int f(int x){switch(x){\n#if 0\ncase 1:\n#else\ncase 2:\n"
     "#endif\ndefault: return 1;}return 0;}\n", 0),
    ("plain switch", "int f(int x){switch(x){case 1: return 2; default: return 1;}}\n", 0),
    ("default only", "int f(int x){switch(x){default: return 1;}}\n", 1),
    ("case in a comment", "int f(int x){switch(x){/* case 1: */ default: return 1;}}\n", 1),
]


def self_test():
    fail = 0
    for name, src, want in SELF_TESTS:
        got = len(scan_text("t.c", src))
        ok = got == want
        fail += not ok
        print(f"{'ok  ' if ok else 'FAIL'} {name}: {got} finding(s), expected {want}")
    print("c4065_scan self-test:", "all passed" if not fail else f"{fail} failed")
    return 1 if fail else 0


def main(argv):
    if argv[1:] == ["--self-test"]:
        return self_test()
    for f in argv[1:]:
        for line in scan_text(f, open(f, errors="replace").read()):
            print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
