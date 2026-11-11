#!/usr/bin/env python3
"""tail_hoist_check.py FILE CALLER HELPER [REV]

The check for Recipe HT (hoist the exit test shared by a selection's branches).
CALLER is the function the step changed, HELPER the static function it added; REV
(default HEAD) is the commit before the step. It compares token by token, with
comments and white space removed, and prints IDENTICAL-MODULO-HOIST when

  - the text of CALLER before the step is CALLER after it with the statement
        if (C) P0 else V=HELPER(args); T
    replaced by
        if (C) { P0 T } else { B }
    where T is `if (V == F) return(V);` and B is HELPER's body between its
    declaration `Type V;` and its closing `return(V);`, each parameter renamed to
    its argument (one identifier per parameter);
  - B holds T once, the text before T has no `return`, `goto`, `break` or
    `continue`, and the text after T (Z) has none of them and does not write V
    (assignment, compound assignment, increment, decrement, `&V`);
  - everything else in CALLER is unchanged.

Recipe HT steps 1 to 4 (outside OpenMP, what B reads and writes, the parameters)
are still checked by reading.
"""
import re
import subprocess
import sys

TOK = re.compile(r"""\"(?:\\.|[^\"\\])*\"|'(?:\\.|[^'\\])*'|[A-Za-z_]\w*|\d[\w.]*"""
                 r"""|==|!=|<=|>=|&&|\|\||->|\+\+|--|<<|>>|[^\s]""")
LEAVE = ("return", "goto", "break", "continue")


def toks(s):
    s = re.sub(r"/\*.*?\*/", " ", s, flags=re.S)
    s = re.sub(r"//[^\n]*", " ", s)
    return TOK.findall(s)


def fn_text(src, name):
    m = re.search(r"^(?:static|MagickExport|MagickPrivate|ModuleExport)\b[^\n;]*[ *]"
                  + re.escape(name) + r"\(.*?^}", src, re.M | re.S)
    if not m:
        sys.exit("no definition of %s" % name)
    return m.group(0)


def match(t, i):
    """Index just after the bracket opened at t[i]."""
    o = t[i]
    c = {"(": ")", "{": "}", "[": "]"}[o]
    d = 0
    for j in range(i, len(t)):
        if t[j] == o:
            d += 1
        elif t[j] == c:
            d -= 1
            if d == 0:
                return j + 1
    sys.exit("unbalanced brackets")


def stmt(t, i):
    """(end index, info) of the statement at t[i]; info = spans of an if statement."""
    w = t[i]
    if w == "{":
        return match(t, i), None
    if w in ("if", "for", "while", "switch"):
        j = match(t, i + 1)
        e, _ = stmt(t, j)
        if w == "if":
            if e < len(t) and t[e] == "else":
                e2, _ = stmt(t, e + 1)
                return e2, (i + 2, j - 1, j, e, e + 1, e2)
            return e, (i + 2, j - 1, j, e, None, None)
        return e, None
    d = 0
    for j in range(i, len(t)):
        if t[j] in "([{":
            d += 1
        elif t[j] in ")]}":
            d -= 1
        elif t[j] == ";" and d == 0:
            return j + 1, None
    sys.exit("no end of statement")


def unbrace(p):
    return p[1:-1] if p and p[0] == "{" and match(p, 0) == len(p) else p


def split_args(t):
    out, cur, d = [], [], 0
    for x in t:
        if x in "([{":
            d += 1
        elif x in ")]}":
            d -= 1
        if x == "," and d == 0:
            out.append(cur)
            cur = []
        else:
            cur.append(x)
    out.append(cur)
    return out


def writes(z, v):
    for i, x in enumerate(z):
        if x != v:
            continue
        nxt = z[i + 1:i + 3]
        prev = z[i - 1:i]
        if nxt[:1] == ["="] or nxt[:1] in (["++"], ["--"]) or prev in (["&"], ["++"], ["--"]):
            return True
        if nxt[:1] and nxt[0] in ("+", "-", "*", "/", "|", "&", "^", "%", "<<", ">>") and nxt[1:2] == ["="]:
            return True
    return False


def fail(msg):
    sys.exit("FAIL: " + msg)


def main():
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    path, caller, helper = sys.argv[1:4]
    rev = sys.argv[4] if len(sys.argv) > 4 else "HEAD"
    root = subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip()
    old_src = subprocess.check_output(["git", "-C", root, "show", "%s:%s" % (rev, path)],
                                      text=True, errors="replace")
    new_src = open(root + "/" + path, encoding="latin-1").read()
    old = toks(fn_text(old_src, caller))
    new = toks(fn_text(new_src, caller))
    h = toks(fn_text(new_src, helper))
    p0 = h.index("(")
    p1 = match(h, p0)
    pnames = [a[-1] for a in split_args(h[p0 + 1:p1 - 1]) if a]
    body = h[p1 + 1:-1]
    k = body.index(";")
    decl = body[:k + 1]
    if len(decl) != 3 or decl[0] != "MagickBooleanType":
        fail("the helper must open with `MagickBooleanType V;`")
    v = decl[1]
    if body[-5:] != ["return", "(", v, ")", ";"]:
        fail("the helper must end with return(%s);" % v)
    b = body[k + 1:-5]
    ci = None
    for i in range(len(new) - 3):
        if new[i:i + 4] == ["else", v, "=", helper]:
            ci = i
            break
    if ci is None:
        fail("no `else %s=%s(...);` in %s" % (v, helper, caller))
    a1 = match(new, ci + 4)
    args = split_args(new[ci + 5:a1 - 1])
    if new[a1] != ";" or len(args) != len(pnames) or any(len(a) != 1 for a in args):
        fail("the call takes one identifier per parameter")
    ren = {pn: a[0] for pn, a in zip(pnames, args)}
    b = [ren.get(x, x) for x in b]
    call_end = a1 + 1
    ti = None
    for i in range(len(b) - 10):
        if b[i:i + 3] == ["if", "(", v] and b[i + 3] == "==" and b[i + 5:i + 11] == [")", "return", "(", v, ")", ";"]:
            if ti is not None:
                fail("the test occurs twice in the helper body")
            ti = i
    if ti is None:
        fail("no test `if (%s == F) return(%s);` in the helper body" % (v, v))
    tpat = b[ti:ti + 11]
    if new[call_end:call_end + 11] != tpat:
        fail("the test does not follow the call unchanged")
    pre, z = b[:ti], b[ti + 11:]
    for nm, part in (("before", pre), ("after", z)):
        bad = [x for x in part if x in LEAVE]
        if bad:
            fail("`%s` %s the test" % (bad[0], nm))
    if writes(z, v):
        fail("the text after the test writes %s" % v)
    found = None
    for i, x in enumerate(new):
        if x == "if" and new[i + 1:i + 2] == ["("]:
            _, info = stmt(new, i)
            if info and info[4] == ci + 1:
                found = (i, info)
    if not found:
        fail("the call is not the else-part of an if statement")
    i0, (c0, c1, t0, t1, _e0, _e1) = found
    cond = new[c0:c1]
    then = unbrace(new[t0:t1])
    if old[:i0] != new[:i0]:
        fail("the text before the statement changed")
    e, info = stmt(old, i0)
    if not info or info[4] is None:
        fail("the old caller has no if/else at the statement")
    oc0, oc1, ot0, ot1, oe0, oe1 = info
    same = {"cond": old[oc0:oc1] == cond,
            "then": unbrace(old[ot0:ot1]) == then + tpat,
            "else": old[oe0:oe1] == ["{"] + b + ["}"],
            "rest": old[e:] == new[call_end + 11:]}
    if not all(same.values()):
        print("DIFFERENT", same)
        sys.exit(1)
    print("IDENTICAL-MODULO-HOIST %s: helper %s holds %d tokens before the test and %d after; test: %s"
          % (caller, helper, len(pre), len(z), " ".join(tpat)))


main()
