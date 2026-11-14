#!/usr/bin/env python3
"""Check a Recipe BC step: the tail of a loop's body moves into a function returning whether it broke.

  loop_tail_check.py FILE CALLER HELPER [--rev REV]

FILE is compared as committed at REV (default HEAD) and as it is in the working tree. CALLER is
the function whose loop lost its tail, HELPER the `static MagickBooleanType` function the step
added directly above it (Recipe BC, steps 3 to 5). The script prints OK when all of these hold:

  - HELPER is static, new, returns MagickBooleanType and has at most four parameters;
  - CALLER holds `if (HELPER(a1,a2,...) != MagickFalse) break;` once, as the last statement of a
    loop body, each argument one identifier and the identifiers HELPER's parameter names in order
    (so no argument is transposed or computed);
  - CALLER's old body is its new body with that statement replaced by HELPER's body, where each
    `return(MagickTrue);` is read as `break;`, each other `return(MagickFalse);` as `continue;`, and
    the helper's last statement `return(MagickFalse);` as the end of the body (the old block may
    also end in an explicit `continue;`, at the end of a loop's body the same step);
  - neither body holds a `#pragma`, and HELPER's body holds no other `return` or a `goto`;
  - the rest of the file is the same token stream with HELPER cut out and CALLER's body replaced
    by a marker, HELPER standing directly above CALLER.

It cannot check that every path of the block ends the iteration through `break;` or `continue;` of
that loop (step 1: read it), types, or the instructions the compiler emits: the build, the oracle
and `tools/obj_modulo.sh` do.
"""
import argparse
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import switch_split_check as S  # noqa: E402

TRUE_RETURN = ["return", "(", "MagickTrue", ")", ";"]
FALSE_RETURN = ["return", "(", "MagickFalse", ")", ";"]


class Refused(Exception):
    """A check of the step failed; the message names it."""


def helper_function(src, name):
    """(function, parameter names) of the helper `name`: static MagickBooleanType, at most four."""
    fn = src.funcs.get(name)
    if fn is None:
        raise Refused("%s is not defined" % name)
    if src.toks[fn.start:fn.start + 2] != ["static", "MagickBooleanType"]:
        raise Refused("%s is not `static MagickBooleanType`" % name)
    names = S.param_names(src.toks, fn)
    if len(names) > 4:
        raise Refused("%s takes %d parameters, at most four" % (name, len(names)))
    return fn, names


def call_site(toks, name, lo, hi):
    """(index of `if`, index after `break ;`, argument tokens) of the one `if (name(...) != MagickFalse) break;`."""
    sites = []
    for i in range(lo, hi):
        if toks[i:i + 4] != ["if", "(", name, "("]:
            continue
        close = S.match_close(toks, i + 3, "(", ")")
        if toks[close + 1:close + 4] == ["!=", "MagickFalse", ")"] and toks[close + 4:close + 6] == ["break", ";"]:
            sites.append((i, close + 6, toks[i + 4:close]))
    if len(sites) != 1:
        raise Refused("%d sites `if (%s(...) != MagickFalse) break;`, expected 1" % (len(sites), name))
    return sites[0]


def check_arguments(arg_tokens, names):
    args = S.split_args(arg_tokens)
    if [a[0] for a in args if len(a) == 1] != names or any(len(a) != 1 for a in args):
        raise Refused("the arguments are not the parameter names %s, in order" % ", ".join(names))


def block_of(helper_tokens, with_continue):
    """The helper's body read as the loop-body block: returns become break and continue."""
    body = list(helper_tokens)
    if body[-5:] != FALSE_RETURN:
        raise Refused("the helper does not end in return(MagickFalse);")
    body = body[:-5] + (["continue", ";"] if with_continue else [])
    out, i = [], 0
    while i < len(body):
        if body[i:i + 5] == TRUE_RETURN:
            out += ["break", ";"]
            i += 5
        elif body[i:i + 5] == FALSE_RETURN:
            out += ["continue", ";"]
            i += 5
        else:
            out.append(body[i])
            i += 1
    return out


def check_no_stray_exit(helper_tokens):
    bad = [t for t in helper_tokens if t in ("goto", "pragma")]
    if bad:
        raise Refused("the helper holds %s" % bad[0])
    returns = [i for i, t in enumerate(helper_tokens) if t == "return"]
    for i in returns:
        if helper_tokens[i:i + 5] not in (TRUE_RETURN, FALSE_RETURN):
            raise Refused("the helper holds a return other than return(MagickTrue/MagickFalse);")


def expected_old_bodies(new, fn_new, helper_tokens, site):
    start, end, _ = site
    toks = new.toks
    for with_continue in (False, True):
        yield (toks[fn_new.body_open + 1:start] + block_of(helper_tokens, with_continue)
               + toks[end:fn_new.body_close])


def check_rest(old, new, caller, helper):
    p = new.funcs[helper]
    cut = new.toks[:p.start] + new.toks[p.body_close + 1:]
    new_fn, old_fn = S.functions(cut)[caller], old.funcs[caller]
    marker = ["<body>"]
    old_rest = old.toks[:old_fn.body_open + 1] + marker + old.toks[old_fn.body_close:]
    new_rest = cut[:new_fn.body_open + 1] + marker + cut[new_fn.body_close:]
    if old_rest != new_rest:
        raise Refused("something outside the caller's body and the helper changed")
    if p.body_close + 1 != new.funcs[caller].start:
        raise Refused("%s does not stand directly above %s" % (helper, caller))


def check(old_text, new_text, caller, helper):
    """A one-line account of the step, or Refused."""
    old, new = S.parse(old_text), S.parse(new_text)
    if helper in old.funcs:
        raise Refused("%s already existed" % helper)
    fn_h, names = helper_function(new, helper)
    helper_tokens = new.toks[fn_h.body_open + 1:fn_h.body_close]
    check_no_stray_exit(helper_tokens)
    fn_old, fn_new = old.funcs[caller], new.funcs[caller]
    site = call_site(new.toks, helper, fn_new.body_open + 1, fn_new.body_close)
    check_arguments(site[2], names)
    if new.toks[site[1]] != "}":
        raise Refused("the call is not the last statement of its block")
    old_body = old.toks[fn_old.body_open + 1:fn_old.body_close]
    if "pragma" in old_body or old_body not in list(expected_old_bodies(new, fn_new, helper_tokens, site)):
        raise Refused("the old body is not the new body with the helper's body in place of the call")
    check_rest(old, new, caller, helper)
    return "%s: %d parameters, %d tokens moved" % (helper, len(names), len(helper_tokens))


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
    ap.add_argument("helper")
    ap.add_argument("--rev", default="HEAD")
    a = ap.parse_args()
    new_text = open(a.file, encoding="utf-8", errors="surrogateescape").read()
    try:
        print("OK: " + check(committed(a.file, a.rev), new_text, a.caller, a.helper))
    except Refused as e:
        print("FAIL: %s" % e)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
