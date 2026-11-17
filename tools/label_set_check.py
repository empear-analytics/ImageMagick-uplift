#!/usr/bin/env python3
"""Check a Recipe LP step: a label set of a `switch` leaves it for a predicate.

  label_set_check.py FILE CALLER PREDICATE [--rev REV]

FILE is compared as committed at REV (default HEAD) and as it is in the working tree.
CALLER is the function whose `switch` lost one group of labels, PREDICATE the function the
step added (Recipe LP, steps 2 to 6). The script prints OK when all of these hold:

  - PREDICATE is `static`, did not exist at REV, has one parameter `const T x`, and its body is
    `switch (E) { <labels>: return(MagickTrue); default: break; } return(MagickFalse);`, no
    label twice;
  - CALLER holds `if (PREDICATE(a) != MagickFalse)` once, where `E` with `x` read as `a` is the
    controlling expression of a `switch` of CALLER's old body, and that `switch` had a group
    whose label set is exactly the predicate's;
  - CALLER's new body is its old body with that group cut out of the `switch` and, directly
    after the `switch`'s closing brace, the `if` followed by the group's body, a last `break;`
    dropped (the fall-out form of step 6), braced when it holds more than one statement;
  - the group's body holds no `break`, `continue`, `goto`, `case` or `#` token once that last
    `break;` is gone, and CALLER's body holds no `#pragma`;
  - every other group of the old `switch` but `default:` ends in a `return` statement, and `a`,
    when it is one identifier, is not assigned, incremented or address-taken in the `switch`;
  - the rest of the file is the same token stream: with PREDICATE's definition cut out and
    CALLER's body replaced by a marker, the two files are equal, and PREDICATE stands directly
    above CALLER.

With --split, the step is Recipe LP step 7: CALLER names an old predicate of the shape above and
PREDICATE the second predicate the step wrote directly above it. The script prints OK when:

  - PREDICATE is static, new, of the same parameter and controlling expression, and has the
    shape above (labels, `default: break;`, `return(MagickFalse);`);
  - CALLER still has that shape except that its tail is `return(PREDICATE(x));` for its own
    parameter `x`;
  - CALLER's new labels followed by PREDICATE's labels are exactly CALLER's old labels, in order,
    each once, both runs of at least two labels;
  - the rest of the file is the same token stream with PREDICATE cut out and CALLER replaced by a
    marker, and PREDICATE stands directly above CALLER.

It cannot check that every path of the group returns (step 1: read it), the types, or the
instructions the compiler emits: the build, the oracle and `tools/obj_modulo.sh` do.
"""
import argparse
import os
import subprocess
import sys
from collections import namedtuple

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import switch_split_check as S  # noqa: E402

TRUE_RETURN = ["return", "(", "MagickTrue", ")", ";"]
FALSE_RETURN = ["return", "(", "MagickFalse", ")", ";"]
DEFAULT_LABELS = ("default :",)
DEFAULT_GROUP = (DEFAULT_LABELS, ("break", ";"))
FORBIDDEN_IN_B = ("break", "continue", "goto", "case", "#")
WRITE_AFTER = ("=", "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=")


Cut = namedtuple("Cut", "src fn found gs k")


class Refused(Exception):
    """A check of the step failed; the message names it."""


def predicate_function(src, name):
    """(function, parameter name) of the predicate `name`: static, one `const T x`."""
    fn = src.funcs.get(name)
    if fn is None:
        raise Refused("%s is not defined" % name)
    if src.toks[fn.start] != "static":
        raise Refused("%s is not static" % name)
    params = S.parameters(src.toks, fn)
    if len(params) != 1 or params[0][0] != "const":
        raise Refused("%s must take one `const T x` parameter" % name)
    return fn, params[0][-1]


def is_membership_run(gs):
    """Two groups: labels returning MagickTrue, then `default: break;`."""
    if len(gs) != 2:
        return False
    return list(gs[0][1]) == TRUE_RETURN and gs[1] == DEFAULT_GROUP


def predicate_groups(src, fn, name, tail=FALSE_RETURN):
    """(switch, groups) of the predicate's body: `switch {...}` then the statement `tail`."""
    found = S.switch_at(src.toks, fn.body_open + 1)
    if not found or src.toks[found[3] + 1:fn.body_close] != tail:
        raise Refused("%s is not `switch {...}` followed by %s" % (name, " ".join(tail)))
    gs = S.groups(src.toks, found[2] + 1, found[3])
    if not is_membership_run(gs):
        raise Refused("%s: groups are not labels, return(MagickTrue), default: break;" % name)
    return found, gs


def predicate_shape(src, name, chained_to=None):
    """(parameter, controlling expression, labels) of the predicate `name`, else Refused.

    chained_to: the predicate whose call ends it (`return(chained_to(x));`), else step 2's
    `return(MagickFalse);`."""
    fn, param = predicate_function(src, name)
    tail = FALSE_RETURN if chained_to is None else ["return", "(", chained_to, "(", param, ")", ")", ";"]
    found, gs = predicate_groups(src, fn, name, tail)
    labels = list(gs[0][0])
    if len(set(labels)) != len(labels):
        raise Refused("%s: a label stands twice" % name)
    return param, tuple(src.toks[found[0] + 2:found[1]]), labels


def call_site(toks, name, lo, hi):
    """(index of `if`, argument tokens) of the one `if (name(a) != MagickFalse)` in toks[lo:hi]."""
    sites = []
    for i in range(lo, hi):
        if toks[i:i + 3] == ["if", "(", name] and toks[i + 3] == "(":
            close = S.match_close(toks, i + 3, "(", ")")
            if toks[close + 1:close + 4] == ["!=", "MagickFalse", ")"]:
                sites.append((i, toks[i + 4:close]))
    if len(sites) != 1:
        raise Refused("%d calls of %s as an `if` condition, expected 1" % (len(sites), name))
    return sites[0]


def substitute(expr, param, arg):
    """expr with each token `param` replaced by the tokens arg."""
    out = []
    for t in expr:
        out.extend(arg if t == param else [t])
    return tuple(out)


def labelled_group(gs, labels):
    """Index of the group of gs whose label set is exactly labels, else None."""
    for k, (group_labels, _) in enumerate(gs):
        if sorted(group_labels) == sorted(labels):
            return k
    return None


def old_switch(src, fn, expr, labels):
    """The Cut of the old caller: the switch on expr with a group holding exactly labels."""
    wanted = [f for f in S.switches(src.toks, fn.body_open + 1, fn.body_close)
              if tuple(src.toks[f[0] + 2:f[1]]) == expr]
    for found in wanted:
        gs = S.groups(src.toks, found[2] + 1, found[3])
        k = labelled_group(gs, labels)
        if k is not None:
            return Cut(src, fn, found, gs, k)
    raise Refused("no switch on that expression has a group with exactly the predicate's labels")


def is_braced(body):
    return body[0] == "{" and S.match_close(list(body), 0, "{", "}") == len(body) - 1


def without_break(tokens):
    """tokens without a last `break ;`."""
    return tokens[:-2] if tokens[-2:] == ["break", ";"] else tokens


def statement_of(body):
    """The group's body as the statement the `if` governs: last `break;` dropped, braced."""
    body = list(body)
    if is_braced(body):
        return ["{"] + without_break(body[1:-1]) + ["}"]
    body = without_break(body)
    complete, tail = S.statements(body)
    if tail or not complete:
        raise Refused("the group's body is not a list of complete statements")
    return body if len(complete) == 1 else ["{"] + body + ["}"]


def check_group_body(statement):
    bad = [t for t in statement if t in FORBIDDEN_IN_B]
    if bad:
        raise Refused("the moved body holds %s" % ", ".join(sorted(set(bad))))


def last_statement(body):
    """The tokens of the last statement of a group body (inside one pair of braces)."""
    body = list(body)
    if is_braced(body):
        body = body[1:-1]
    complete, _ = S.statements(body)
    if not complete:
        raise Refused("a group has no statement")
    return complete[-1]


def ends_in_return(labels, body):
    return labels == DEFAULT_LABELS or last_statement(body)[:1] == ["return"]


def check_other_groups(cut):
    for j, (labels, body) in enumerate(cut.gs):
        if j != cut.k and not ends_in_return(labels, body):
            raise Refused("the group %s does not end in a return" % labels[0])


def writes_at(toks, i, name):
    """True when toks[i] is `name` written (assigned, incremented) or taken by `&` or `++`."""
    if toks[i] == name and toks[i + 1] in WRITE_AFTER:
        return True
    return toks[i] in ("++", "--", "&") and toks[i + 1] == name


def check_not_written(arg, switch_tokens):
    if len(arg) == 1 and any(writes_at(switch_tokens, i, arg[0]) for i in range(len(switch_tokens) - 1)):
        raise Refused("%s is written or its address taken in the switch" % arg[0])


def expected_body(cut, pred_name, arg):
    """The old body with group k cut out and the predicate's `if` after the switch."""
    toks, found = cut.src.toks, cut.found
    inner = []
    for j, (labels, body) in enumerate(cut.gs):
        if j != cut.k:
            inner += [w for label in labels for w in label.split(" ")] + list(body)
    cond = ["if", "(", pred_name, "("] + arg + [")", "!=", "MagickFalse", ")"]
    return (toks[cut.fn.body_open + 1:found[2] + 1] + inner + ["}"] + cond
            + statement_of(cut.gs[cut.k][1]) + toks[found[3] + 1:cut.fn.body_close])


def check_rest_of_file(old, new, caller, pred):
    """Tokens outside the caller's body and the predicate's definition are unchanged."""
    p = new.funcs[pred]
    cut = new.toks[:p.start] + new.toks[p.body_close + 1:]
    new_fn = S.functions(cut)[caller]
    old_fn = old.funcs[caller]
    marker = ["<body>"]
    old_rest = old.toks[:old_fn.body_open + 1] + marker + old.toks[old_fn.body_close:]
    new_rest = cut[:new_fn.body_open + 1] + marker + cut[new_fn.body_close:]
    if old_rest != new_rest:
        raise Refused("something outside the caller's body and the predicate changed")
    if p.body_close + 1 != new.funcs[caller].start:
        raise Refused("%s does not stand directly above %s" % (pred, caller))


def check(old_text, new_text, caller, pred_name):
    """A one-line account of the step, or Refused."""
    old, new = S.parse(old_text), S.parse(new_text)
    if pred_name in old.funcs:
        raise Refused("%s already existed" % pred_name)
    param, expr, labels = predicate_shape(new, pred_name)
    fn_old, fn_new = old.funcs[caller], new.funcs[caller]
    if "pragma" in old.toks[fn_old.body_open:fn_old.body_close]:
        raise Refused("the caller holds a #pragma")
    _, arg = call_site(new.toks, pred_name, fn_new.body_open + 1, fn_new.body_close)
    cut = old_switch(old, fn_old, substitute(expr, param, arg), labels)
    check_group_body(statement_of(cut.gs[cut.k][1]))
    check_other_groups(cut)
    check_not_written(arg, old.toks[cut.found[0]:cut.found[3] + 1])
    if expected_body(cut, pred_name, arg) != new.toks[fn_new.body_open + 1:fn_new.body_close]:
        raise Refused("the new body is not the old body with the group moved after the switch")
    check_rest_of_file(old, new, caller, pred_name)
    return "%s: %d labels, group of %d tokens moved, %d other groups stay" % (
        pred_name, len(labels), len(cut.gs[cut.k][1]), len(cut.gs) - 1)


def check_split_rest(old, new, first, second):
    """Tokens outside both predicates are unchanged; the second stands directly above the first."""
    p = new.funcs[second]
    cut = new.toks[:p.start] + new.toks[p.body_close + 1:]
    new_first, old_first = S.functions(cut)[first], old.funcs[first]
    marker = ["<predicate>"]
    old_rest = old.toks[:old_first.start] + marker + old.toks[old_first.body_close + 1:]
    new_rest = cut[:new_first.start] + marker + cut[new_first.body_close + 1:]
    if old_rest != new_rest:
        raise Refused("something outside the two predicates changed")
    if p.body_close + 1 != new.funcs[first].start:
        raise Refused("%s does not stand directly above %s" % (second, first))


def split_shapes(old, new, first, second):
    """The shapes of the old predicate, the chained first and the second; one parameter and
    controlling expression throughout."""
    shapes = (predicate_shape(old, first), predicate_shape(new, first, second),
              predicate_shape(new, second))
    if len({(shape[0], shape[1]) for shape in shapes}) != 1:
        raise Refused("the predicates differ in parameter or controlling expression")
    return shapes


def check_split_labels(old_labels, first_labels, second_labels):
    if first_labels + second_labels != old_labels:
        raise Refused("the two label runs together are not the old run, in order")
    if min(len(first_labels), len(second_labels)) < 2:
        raise Refused("a run of fewer than two labels")


def check_split(old_text, new_text, first, second):
    """A one-line account of an LP step 7 split, or Refused."""
    old, new = S.parse(old_text), S.parse(new_text)
    if second in old.funcs:
        raise Refused("%s already existed" % second)
    shape_old, shape_first, shape_second = split_shapes(old, new, first, second)
    check_split_labels(shape_old[2], shape_first[2], shape_second[2])
    check_split_rest(old, new, first, second)
    return "%s keeps %d labels, %s takes %d" % (first, len(shape_first[2]), second, len(shape_second[2]))


def committed(path, rev):
    out = subprocess.run(["git", "show", "%s:%s" % (rev, path)], capture_output=True,
                         text=True, errors="surrogateescape")
    if out.returncode != 0:
        raise SystemExit("cannot read %s at %s" % (path, rev))
    return out.stdout


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("file")
    ap.add_argument("caller")
    ap.add_argument("predicate")
    ap.add_argument("--rev", default="HEAD")
    ap.add_argument("--split", action="store_true", help="check a Recipe LP step 7 split")
    a = ap.parse_args()
    new_text = open(a.file, encoding="utf-8", errors="surrogateescape").read()
    try:
        step = check_split if a.split else check
        print("OK: " + step(committed(a.file, a.rev), new_text, a.caller, a.predicate))
    except Refused as e:
        print("FAIL: %s" % e)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
