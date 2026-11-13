#!/usr/bin/env python3
"""Check a Recipe SD step on a function no oracle case executes.

  switch_split_check.py FILE CALLER HELPER [HELPER ...] [--rev REV]

FILE is compared as committed at REV (default HEAD) and as it is in the working tree.
CALLER is the function whose `switch` the step split; the HELPERs are the functions the
step added, in the order the calls chain: the one CALLER's `default:` group calls first,
then the one that helper's `default:` group calls, and so on (Recipe SD steps 1 to 6, and
the `Targets` form of step 8). The script reads the groups of every `switch` in the chain
(a group is a run of labels and the body after them) and prints OK when all of these hold:

  - the added functions are exactly the HELPERs, and nothing else of the file changed:
    with the HELPER definitions cut out and each `switch (E) { ... }` of CALLER replaced
    by a marker, the two files are the same token stream;
  - every `switch` in the chain, the caller's and each helper's, controls on the same
    expression text `E`, and a helper's body is its `switch` and nothing but bindings
    `T *const v = p->v;` (a member bound under its own name, a member of the `const`
    targets typedef `T` that the caller's `const T x = { &m1, &m2, ... };` initialiser
    fills by the addresses of the variables of those names, in order);
  - a `default:` group that is not the innermost holds only the call of the next helper,
    `Next(a1,a2,...);` and `break;`, each argument the parameter of its position by name
    (`&p` for a pointer parameter `p` of the same name, or `p` passed on), so no argument is
    transposed;
  - the label sets partition the original's: flattening the chain (each link replaced by the
    groups of the helper it calls) gives, in the working tree, exactly the groups the
    original's chain had, label for label (every label in exactly one group of exactly one
    `switch`, `default:` in the innermost one, no label lost, added or doubled);
  - every group's body is the original's, token for token, once each bound member `(*v)` is
    read as `v` and a `&p` of a pointer parameter as `p` (the rewrite Recipe O step 5 and
    Recipe SD step 8 give); a group the caller keeps is the original's as written;
  - no moved group holds a `return`, `continue` or `goto`, and within each `switch` the
    groups keep their original order.

It cannot check types, the precedence of `*v` against `.`, `->` and `[]` (a wrong one does
not compile for the members Recipe SD carries), or what the groups read from the caller
beyond the arguments of the link calls: the build, `tools/obj_modulo.sh` and reading do.
"""
import re
import subprocess
import sys

TOKEN = re.compile(
    r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|\d[\w.]*|->|<<=|>>=|\+\+|--|'
    r'<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^]=|\S')


def strip_comments(text):
    out = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            out.append(text[i:j + 1])
            i = j + 1
        elif text.startswith("/*", i):
            j = text.index("*/", i + 2)
            out.append(" ")
            i = j + 2
        elif text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        else:
            out.append(c)
            i += 1
    return "".join(out)


def tokens(text):
    return TOKEN.findall(text)


def match_close(toks, i, open_, close):
    """Index of the token closing the bracket opened at toks[i]."""
    depth = 0
    for j in range(i, len(toks)):
        if toks[j] == open_:
            depth += 1
        elif toks[j] == close:
            depth -= 1
            if depth == 0:
                return j
    raise SystemExit("FAIL: unbalanced %s at token %d" % (open_, i))


def functions(toks):
    """name -> (start, params_start, params_end, body_open, body_close), top-level definitions."""
    found = {}
    i = 0
    n = len(toks)
    depth = 0
    while i < n:
        t = toks[i]
        if t == "{":
            depth += 1
        elif t == "}":
            depth -= 1
        elif depth == 0 and t == "(" and i > 0 and re.match(r"[A-Za-z_]\w*$", toks[i - 1]):
            close = match_close(toks, i, "(", ")")
            if close + 1 < n and toks[close + 1] == "{":
                body_close = match_close(toks, close + 1, "{", "}")
                start = i - 1
                while start > 0 and toks[start - 1] not in (";", "}", "{"):
                    start -= 1
                found[toks[i - 1]] = (start, i, close, close + 1, body_close)
                i = body_close + 1
                continue
        i += 1
    return found


def switches(toks, lo, hi):
    """Top-level (not nested in another switch) `switch (E) { ... }` in toks[lo:hi]."""
    found = []
    i = lo
    while i < hi:
        if toks[i] == "switch" and toks[i + 1] == "(":
            pc = match_close(toks, i + 1, "(", ")")
            bc = match_close(toks, pc + 1, "{", "}")
            found.append((i, pc, pc + 1, bc))
            i = bc
        i += 1
    return found


def groups(toks, lo, hi):
    """The groups of a switch body toks[lo:hi]: (labels, body tokens)."""
    out = []
    labels = []
    body = []
    i = lo
    depth = 0
    while i < hi:
        t = toks[i]
        if depth == 0 and t in ("case", "default"):
            if body:
                out.append((tuple(labels), tuple(body)))
                labels, body = [], []
            j = i + 1
            while toks[j] != ":":
                j += 1
            labels.append(" ".join(toks[i:j + 1]))
            i = j + 1
            continue
        if t == "{":
            depth += 1
        elif t == "}":
            depth -= 1
        body.append(t)
        i += 1
    if labels:
        out.append((tuple(labels), tuple(body)))
    return out


def split_args(toks):
    args, cur, depth = [], [], 0
    for t in toks:
        if t in "([{":
            depth += 1
        elif t in ")]}":
            depth -= 1
        if t == "," and depth == 0:
            args.append(cur)
            cur = []
        else:
            cur.append(t)
    if cur:
        args.append(cur)
    return args


def param_names(toks, fn):
    s, ps, pe, bo, bc = fn
    names = []
    for a in split_args(toks[ps + 1:pe]):
        names.append(a[-1])
    return names


def normalise(body, bound, ptr_params):
    """(*v) -> v for bound members, &p -> p for pointer parameters."""
    out = []
    i = 0
    while i < len(body):
        if (body[i] == "(" and i + 3 < len(body) and body[i + 1] == "*"
                and body[i + 2] in bound and body[i + 3] == ")"):
            out.append(body[i + 2])
            i += 4
        elif body[i] == "&" and i + 1 < len(body) and body[i + 1] in ptr_params:
            out.append(body[i + 1])
            i += 2
        else:
            out.append(body[i])
            i += 1
    return tuple(out)


class Chain:
    """The `switch` of a function: its controlling expression, bindings and groups."""

    def __init__(self, toks, funcs, name, problems, strict):
        self.name = name
        fn = funcs[name]
        self.fn = fn
        s, ps, pe, bo, bc = fn
        sw = switches(toks, bo + 1, bc)
        if not sw:
            problems.append("%s has no switch" % name)
            self.ok = False
            return
        self.ok = True
        self.sw = sw[0]
        i, pc, bopen, bclose = self.sw
        self.expr = tuple(toks[i + 2:pc])
        self.params = param_names(toks, fn)
        self.ptr_params = set()
        for a in split_args(toks[ps + 1:pe]):
            if "*" in a:
                self.ptr_params.add(a[-1])
        # the body is bindings and the switch, nothing else
        rest = toks[bo + 1:i] + toks[bclose + 1:bc] if strict else []
        self.bound = {}
        decls = []
        cur = []
        for t in rest:
            cur.append(t)
            if t == ";":
                decls.append(cur)
                cur = []
        if cur:
            problems.append("%s: statements outside its switch: %s" % (name, " ".join(cur)))
        self.rest_ok = True
        for d in decls:
            m = re.match(r"^(?:[\w]+ )+\* const (\w+) = (\w+) -> (\w+) ;$", " ".join(d))
            if not m:
                problems.append("%s: not a binding: %s" % (name, " ".join(d)))
                continue
            v, p, member = m.groups()
            if v != member:
                problems.append("%s: %s bound from member %s" % (name, v, member))
            if p not in self.params:
                problems.append("%s: binding from %s, not a parameter" % (name, p))
            self.bound[v] = (p, member)
        self.groups = groups(toks, bopen + 1, bclose)
        self.toks = toks


def link_target(norm_body, names):
    """Next helper name when the group is `{ Next(...); break; }`, else None."""
    b = list(norm_body)
    if b and b[0] == "{":
        b = b[1:-1] if b[-1] == "}" else b
    if len(b) >= 4 and b[0] in names and b[1] == "(" and b[-4] == ")" and b[-3] == ";" \
            and b[-2] == "break" and b[-1] == ";":
        return b[0], b[2:-4]
    return None


def flatten(toks, funcs, name, expr, problems, names, tag, ptr_all, top=True):
    """[(function, group index, labels, normalised body)] with links expanded."""
    ch = Chain(toks, funcs, name, problems, not top)
    if not ch.ok:
        return [], []
    if ch.expr != expr and expr is not None:
        problems.append("%s %s: controls on `%s`, not `%s`" %
                        (tag, name, " ".join(ch.expr), " ".join(expr)))
    out = []
    seen = [ch]
    for k, (labels, body) in enumerate(ch.groups):
        nb = normalise(body, set(ch.bound), ptr_all)
        lk = link_target(nb, names)
        if lk and labels == ("default :",) and lk[0] in funcs and lk[0] != name:
            nxt = lk[0]
            sub, subch = flatten(toks, funcs, nxt, ch.expr, problems, names, tag, ptr_all, False)
            # arguments by position: the next helper's parameter of the same name
            nparams = param_names(toks, funcs[nxt])
            args = split_args(list(lk[1]))
            if len(args) != len(nparams):
                problems.append("%s: call of %s passes %d arguments for %d parameters" %
                                (name, nxt, len(args), len(nparams)))
            else:
                nptr = Chain(toks, funcs, nxt, [], False).ptr_params
                for a, p in zip(args, nparams):
                    if a != [p] and not (a == ["&", p] and p in nptr):
                        problems.append("%s: call of %s passes `%s` where the parameter "
                                        "is %s" % (name, nxt, " ".join(a), p))
            out.append((name, k, None, ("LINK", nxt)))
            out.extend(sub)
            seen.extend(subch)
        else:
            out.append((name, k, labels, nb))
    return out, seen


def cut(toks, start, end, marker):
    return toks[:start] + [marker] + toks[end + 1:]


def check(old_text, new_text, caller, helpers):
    problems = []
    otoks = tokens(strip_comments(old_text))
    ntoks = tokens(strip_comments(new_text))
    ofuncs = functions(otoks)
    nfuncs = functions(ntoks)
    if caller not in ofuncs or caller not in nfuncs:
        return ["%s not found on both sides" % caller], ""
    added = sorted(set(nfuncs) - set(ofuncs))
    if added != sorted(helpers):
        problems.append("functions added: %s, expected %s" % (added, sorted(helpers)))
    gone = sorted(set(ofuncs) - set(nfuncs))
    if gone:
        problems.append("functions removed: %s" % gone)
    if problems:
        return problems, ""
    names = set(helpers) | set(ofuncs)
    # the typedef initialiser of the targets struct is read from the caller
    chain_names = [caller] + list(helpers)
    # the controlling expression: the new caller's switch that holds the call of the first helper
    nch = Chain(ntoks, nfuncs, caller, problems, False)
    if not nch.ok:
        return problems, ""
    expr = nch.expr
    ptr_all = set()
    for h in helpers:
        ptr_all |= Chain(ntoks, nfuncs, h, [], False).ptr_params
    new_flat, new_chains = flatten(ntoks, nfuncs, caller, expr, problems, names, "new", ptr_all)
    old_flat, old_chains = flatten(otoks, ofuncs, caller, expr, problems, names, "old", ptr_all)
    # every helper must be reached by the chain
    reached = {c.name for c in new_chains}
    for h in helpers:
        if h not in reached:
            problems.append("%s is not in the chain of %s" % (h, caller))
    # the label partition: flattened leaf groups, label for label
    def leaves(flat):
        return [(f, k, lab, b) for (f, k, lab, b) in flat if lab is not None]
    old_leaves, new_leaves = leaves(old_flat), leaves(new_flat)
    for tag, lv in (("original", old_leaves), ("result", new_leaves)):
        seen = {}
        for f, k, lab, b in lv:
            for l in lab:
                if l in seen:
                    problems.append("%s: label `%s` in two groups (%s and %s)" %
                                    (tag, l, seen[l], f))
                seen[l] = f
    old_map = {lab: b for (f, k, lab, b) in old_leaves}
    new_map = {lab: b for (f, k, lab, b) in new_leaves}
    old_labels = {l for lab in old_map for l in lab}
    new_labels = {l for lab in new_map for l in lab}
    for l in sorted(old_labels - new_labels):
        problems.append("label `%s` lost" % l)
    for l in sorted(new_labels - old_labels):
        problems.append("label `%s` added" % l)
    for lab in old_map:
        if lab in new_map and old_map[lab] != new_map[lab]:
            problems.append("group %s: body differs\n    was: %s\n    now: %s" %
                            (" ".join(lab), " ".join(old_map[lab]), " ".join(new_map[lab])))
        elif lab not in new_map and set(lab) <= new_labels:
            problems.append("group %s: its labels are split across groups" % " ".join(lab))
    # a group the caller keeps is unchanged, as written (the normalisation above is for moved ones)
    oraw = {lab: body for (lab, body) in Chain(otoks, ofuncs, caller, [], False).groups}
    for lab, body in nch.groups:
        if lab != ("default :",) and lab in oraw and oraw[lab] != body:
            problems.append("kept group %s was edited" % " ".join(lab))
    # no moved group may leave the switch other than by break
    for f, k, lab, b in new_leaves:
        if f == caller:
            continue
        for bad in ("return", "continue", "goto"):
            if bad in b:
                problems.append("%s: group %s holds `%s`" % (f, " ".join(lab), bad))
    # source order within each switch
    order = {lab: n for n, (f, k, lab, b) in enumerate(old_leaves)}
    by_fn = {}
    for f, k, lab, b in new_leaves:
        by_fn.setdefault(f, []).append(order.get(lab, -1))
    for f, idx in by_fn.items():
        if idx != sorted(idx):
            problems.append("%s: groups are not in their original order" % f)
    # the bindings: the members of the targets typedef, filled by the caller's initialiser
    for c in new_chains:
        for v, (p, member) in c.bound.items():
            ps = param_names(ntoks, nfuncs[c.name])
            s, pss, pe, bo, bc = nfuncs[c.name]
            ptype = None
            for a in split_args(ntoks[pss + 1:pe]):
                if a[-1] == p:
                    ptype = a[-2] if a[-2] != "*" else a[-3]
            if ptype is None:
                problems.append("%s: parameter %s has no type" % (c.name, p))
                continue
            td = re.search(r"typedef struct _\w+\s*\{([^{}]*)\}\s*" + re.escape(ptype) + r"\s*;",
                           strip_comments(new_text))
            members = []
            if td:
                for decl in td.group(1).split(";"):
                    for part in decl.split(","):
                        w = re.findall(r"\w+", part)
                        if w:
                            members.append(w[-1])
            if member not in members:
                problems.append("%s: %s is not a member of %s" % (c.name, member, ptype))
                continue
            init = re.findall(r"const\s+" + re.escape(ptype) + r"\s+\w+\s*=\s*\{([^}]*)\}",
                              strip_comments(new_text))
            if not init:
                problems.append("no const %s initialiser in the file" % ptype)
            elif [x.strip() for x in init[0].split(",")] != ["&" + m for m in members]:
                problems.append("the %s initialiser does not name its members by address in "
                                "order" % ptype)
    # nothing else changed: cut the helpers and the caller's switches, compare the rest
    def residue(toks, funcs, drop):
        t = list(toks)
        spans = []
        for name in drop:
            if name in funcs:
                s, ps, pe, bo, bc = funcs[name]
                spans.append((s, bc, None))
        s, ps, pe, bo, bc = funcs[caller]
        for (i, pc, bopen, bclose) in switches(toks, bo + 1, bc):
            spans.append((i, bclose, "SWITCH"))
        out = []
        pos = 0
        for a, b, mk in sorted(spans):
            out.extend(t[pos:a])
            if mk:
                out.append(mk)
            pos = b + 1
        out.extend(t[pos:])
        return out
    ores = residue(otoks, ofuncs, [])
    nres = residue(ntoks, nfuncs, helpers)
    if ores != nres:
        for k, (a, b) in enumerate(zip(ores, nres)):
            if a != b:
                problems.append("the file differs outside the helpers and the switch near "
                                "`%s` / `%s`" % (" ".join(ores[max(0, k - 4):k + 4]),
                                                 " ".join(nres[max(0, k - 4):k + 4])))
                break
        else:
            problems.append("the file differs outside the helpers and the switch "
                            "(length %d / %d)" % (len(ores), len(nres)))
    summary = ("%d labels in %d groups before, %d labels in %d groups across %d switches "
               "after" % (len(old_labels), len(old_leaves), len(new_labels), len(new_leaves),
                          len(new_chains)))
    return problems, summary


def main(argv):
    rev = "HEAD"
    args = []
    i = 1
    while i < len(argv):
        if argv[i] == "--rev":
            rev = argv[i + 1]
            i += 2
        else:
            args.append(argv[i])
            i += 1
    if len(args) < 3:
        print(__doc__)
        return 2
    path, caller, helpers = args[0], args[1], args[2:]
    old = subprocess.run(["git", "show", "%s:%s" % (rev, path)], capture_output=True,
                         text=True, check=True).stdout
    new = open(path).read()
    problems, summary = check(old, new, caller, helpers)
    if problems:
        for p in dict.fromkeys(problems):
            print("FAIL: " + p)
        return 1
    print("OK %s: %s" % (caller, summary))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
