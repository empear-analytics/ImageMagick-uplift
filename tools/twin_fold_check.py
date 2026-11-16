#!/usr/bin/env python3
"""twin_fold_check.py: check that a merged function folds back to each twin it replaced.

Recipes TI and TH merge two near-identical static functions ("copies") into one function
that reads a constant selector from a struct and writes each differing run as
`((side == NameX) ? runX : runY)`. Their last step asks for a script check: folding every
such conditional to copy X's run, and removing the `const` bindings of the struct's
members, must give copy X's token sequence. This is that script.

    python3 tools/twin_fold_check.py BASE CAND --merged NewName \\
        --copy OldLSB=XPackLSBFirst --copy OldMSB=XPackMSBFirst [--selector side]

BASE and CAND are C files; either may be REV:path (read with `git show`). The copies are
read from BASE, the merged function from CAND. For each copy the script

  1. takes the merged function's body and drops its leading bindings, the declarations
     `const T [*const] name = s->member;` whose initialiser is a member of a struct pointer
     (one per member the body reads), and the selector's binding with them;
  2. folds every `( ( SEL == K ) ? A : B )` to A when K is the copy's constant, to B
     otherwise (nested conditionals and parentheses inside A and B are balanced);
  3. compares the result with the copy's body token by token, comments dropped and the
     run of leading plain declarations (`int x;`, `unsigned int nibble;`: no `=`, no
     call) compared as a set, since the copies may declare them in another order.

It prints `MATCH <copy>` or `MISMATCH <copy>` with the first differing tokens, and exits
1 when any copy mismatches or a function is missing. tools/test_twin_fold_check.py is its
planted-defect test.
"""
import argparse
import os
import re
import subprocess
import sys

TOKEN = re.compile(r'''
    (?P<comment>/\*.*?\*/|//[^\n]*)
  | (?P<string>"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')
  | (?P<number>\.?\d[\w.]*(?:[eEpP][+-]\d+)?)
  | (?P<ident>[A-Za-z_]\w*)
  | (?P<op>->|\+\+|--|<<=|>>=|<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^!~<>=]=?|[()\[\]{};,.?:#])
  | (?P<space>\s+)
''', re.S | re.X)


def read(spec):
    if not os.path.exists(spec) and ':' in spec:
        return subprocess.run(['git', 'show', spec], check=True, capture_output=True,
                              text=True, errors='surrogateescape').stdout
    with open(spec, encoding='utf-8', errors='surrogateescape') as f:
        return f.read()


def tokens(text):
    out = []
    pos = 0
    while pos < len(text):
        m = TOKEN.match(text, pos)
        if not m:
            raise SystemExit('cannot tokenize at: %r' % text[pos:pos + 40])
        pos = m.end()
        if m.lastgroup in ('space', 'comment'):
            continue
        out.append(m.group())
    return out


def body(text, name):
    """Tokens of the body (between the outer braces) of function NAME."""
    m = re.search(r'^(?:static|MagickExport|MagickPrivate)[^;{]*?[\s*]' + re.escape(name)
                  + r'\(', text, re.M)
    if not m:
        return None
    toks = tokens(text[m.start():])
    start = toks.index('{')
    depth = 0
    for i in range(start, len(toks)):
        if toks[i] == '{':
            depth += 1
        elif toks[i] == '}':
            depth -= 1
            if depth == 0:
                return toks[start + 1:i]
    return None


def statements(toks):
    """Split a token list at top-level ';' into statements (each ending with ';')."""
    out, cur, depth = [], [], 0
    for t in toks:
        cur.append(t)
        if t in '([{':
            depth += 1
        elif t in ')]}':
            depth -= 1
        elif t == ';' and depth == 0:
            out.append(cur)
            cur = []
    return out, cur


def is_binding(stmt):
    # const T [*] [const] name = s -> member ;
    return (len(stmt) >= 7 and stmt[0] == 'const' and stmt[-1] == ';' and
            stmt[-3] == '->' and stmt[-5] == '=' and
            re.match(r'[A-Za-z_]\w*$', stmt[-4]) and re.match(r'[A-Za-z_]\w*$', stmt[-2]))


def is_plain_decl(stmt):
    return (stmt[-1] == ';' and '=' not in stmt and '(' not in stmt and
            all(re.match(r'[A-Za-z_]\w*$|[*,;]$', t) for t in stmt) and len(stmt) >= 3)


def strip_bindings(toks):
    stmts, _ = statements(toks)
    i = n = 0
    while i < len(stmts) and is_binding(stmts[i]):
        n += len(stmts[i])
        i += 1
    return toks[n:]


def normalise(toks):
    """Leading plain declarations as a sorted block, then the rest in order."""
    stmts, _ = statements(toks)
    decls, n = [], 0
    for s in stmts:
        if not is_plain_decl(s):
            break
        decls.append(' '.join(s))
        n += len(s)
    return sorted(decls), toks[n:]


def fold(toks, selector, const):
    """Fold every ( ( selector == K ) ? A : B ) to A when K == const, else B."""
    out = []
    i = 0
    while i < len(toks):
        if (toks[i:i + 3] == ['(', '(', selector] and i + 6 < len(toks) and
                toks[i + 3] == '==' and toks[i + 5] == ')' and toks[i + 6] == '?'):
            k = toks[i + 4]
            j = i + 7
            depth, nest, colon = 0, 0, None
            while j < len(toks):
                t = toks[j]
                if t in '([{':
                    depth += 1
                elif t in ')]}':
                    if depth == 0:
                        break
                    depth -= 1
                elif t == '?' and depth == 0:
                    nest += 1
                elif t == ':' and depth == 0:
                    if nest == 0 and colon is None:
                        colon = j
                    else:
                        nest -= 1
                j += 1
            if colon is None or j >= len(toks):
                raise SystemExit('unbalanced selection at token %d' % i)
            run = toks[i + 7:colon] if k == const else toks[colon + 1:j]
            out.extend(fold(run, selector, const))
            i = j + 1
            continue
        out.append(toks[i])
        i += 1
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('base')
    ap.add_argument('cand')
    ap.add_argument('--merged', required=True)
    ap.add_argument('--copy', action='append', required=True, metavar='NAME=CONST')
    ap.add_argument('--selector', default='side')
    args = ap.parse_args(argv)
    base, cand = read(args.base), read(args.cand)
    merged = body(cand, args.merged)
    if merged is None:
        print('MISSING merged function %s in %s' % (args.merged, args.cand))
        return 1
    merged = strip_bindings(merged)
    status = 0
    for spec in args.copy:
        name, const = spec.split('=', 1)
        copy = body(base, name)
        if copy is None:
            print('MISSING copy %s in %s' % (name, args.base))
            status = 1
            continue
        got_decls, got = normalise(fold(merged, args.selector, const))
        want_decls, want = normalise(copy)
        if got_decls == want_decls and got == want:
            print('MATCH %s (%d tokens, %d leading declarations)' %
                  (name, len(want), len(want_decls)))
            continue
        status = 1
        if got_decls != want_decls:
            print('MISMATCH %s: leading declarations %s != %s' % (name, got_decls, want_decls))
        else:
            k = next((i for i, (a, b) in enumerate(zip(got, want)) if a != b),
                     min(len(got), len(want)))
            print('MISMATCH %s at token %d: merged %s | copy %s' % (
                name, k, ' '.join(got[max(0, k - 8):k + 8]),
                ' '.join(want[max(0, k - 8):k + 8])))
    return status


if __name__ == '__main__':
    sys.exit(main())
