#!/usr/bin/env python3
"""Check a Recipe SG rewrite token by token.

  guard_spread_check.py FILE FUNCTION GUARD [GUARD ...] [--rev REV]

FUNCTION is the function the step changed; each GUARD is the text of one guard `G` of
`if (G)` that the step spread (the text between the parentheses of the `if`, whitespace
free to differ). The script reads FUNCTION from REV (default HEAD) and from the working
tree, applies Recipe SG to the old text mechanically, and compares the result with the
new text token by token (comments dropped). For every `if (G) { ... }` of the old
function whose guard is one of the GUARDs it requires the body to be

  (a) one chain `if (A1) X1 else if (A2) X2 ... else if (An) Xn` with no final `else`
      and n >= 2: the expected rewrite is `if ((G) && (A1)) X1 else if ((G) && (A2))
      X2 ...`, the outer braces gone and each Xk unchanged; or
  (b) statements S followed by one `if (B) T` with no `else`: the expected rewrite is
      `if (G) S` (S in braces when it is more than one statement, or one compound
      statement) followed by `if ((G) && (B)) T`;

and prints OK when the rewritten old text equals the new text, or FAIL with the first
token where they differ. A guard of the old function that has no such body is left as it
is, and so must stay in the new text. It does not check that G has no side effect, or
that S writes nothing G reads (Recipe SG steps 1 and 2: by reading).
"""
import re
import subprocess
import sys

TOK = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|\d[\w.]*|->|&&|\|\||[<>!=+\-*/%&|^]=|<<=|>>=|<<|>>|\+\+|--|\S')


def tokens(text):
    text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
    text = re.sub(r'//[^\n]*', ' ', text)
    return TOK.findall(text)


def function_text(src, name):
    m = re.search(r'^[A-Za-z_][^\n;{]*\b%s\(' % re.escape(name), src, re.M)
    while m:
        i = m.start()
        j = src.index('\n{', i) if '\n{' in src[i:] else -1
        k = src.find(';', m.end())
        if j != -1 and (k == -1 or j < k):
            e = src.index('\n}\n', j)
            return src[i:e + 3]
        m = re.search(r'^[A-Za-z_][^\n;{]*\b%s\(' % re.escape(name), src[m.end():], re.M)
    raise SystemExit("function %s not found" % name)


def match(toks, i, op, cl):
    d = 0
    while i < len(toks):
        if toks[i] == op:
            d += 1
        elif toks[i] == cl:
            d -= 1
            if d == 0:
                return i
        i += 1
    raise SystemExit("unbalanced")


def stmt_end(toks, i):
    """Index just past the statement starting at i."""
    t = toks[i]
    if t == '{':
        return match(toks, i, '{', '}') + 1
    if t == 'if':
        c = match(toks, i + 1, '(', ')')
        e = stmt_end(toks, c + 1)
        if e < len(toks) and toks[e] == 'else':
            return stmt_end(toks, e + 1)
        return e
    d = 0
    while i < len(toks):
        if toks[i] in '([{':
            d += 1
        elif toks[i] in ')]}':
            d -= 1
        elif toks[i] == ';' and d == 0:
            return i + 1
        i += 1
    raise SystemExit("no end of statement")


def split_chain(toks):
    """toks: one if-else-if chain without final else. Returns [(A tokens, X tokens)] or None."""
    out = []
    i = 0
    while True:
        if i >= len(toks) or toks[i] != 'if':
            return None
        c = match(toks, i + 1, '(', ')')
        a = toks[i + 2:c]
        e = stmt_end(toks, c + 1)
        x = toks[c + 1:e]
        out.append((a, x))
        if e == len(toks):
            return out
        if toks[e] != 'else':
            return None
        i = e + 1
        if i < len(toks) and toks[i] != 'if':
            return None


def rewrite(toks, guards):
    out = []
    i = 0
    while i < len(toks):
        if toks[i] == 'if' and i + 1 < len(toks) and toks[i + 1] == '(':
            c = match(toks, i + 1, '(', ')')
            g = toks[i + 2:c]
            if g in guards and c + 1 < len(toks) and toks[c + 1] == '{':
                e = match(toks, c + 1, '{', '}')
                body = toks[c + 2:e]
                if e + 1 < len(toks) and toks[e + 1] == 'else':
                    out.extend(toks[i:e + 1])
                    i = e + 1
                    continue
                # (a) one chain
                ch = split_chain(body) if body and body[0] == 'if' and stmt_end(body, 0) == len(body) else None
                if ch is not None and len(ch) >= 2:
                    first = True
                    for a, x in ch:
                        if not first:
                            out.append('else')
                        first = False
                        out += ['if', '(', '('] + g + [')', '&&', '('] + a + [')', ')'] + x
                    i = e + 1
                    continue
                # (b) S then one if without else
                j = 0
                starts = []
                while j < len(body):
                    starts.append(j)
                    j = stmt_end(body, j)
                last = starts[-1] if starts else None
                if last is not None and len(starts) >= 2 and body[last] == 'if':
                    c2 = match(body, last + 1, '(', ')')
                    e2 = stmt_end(body, last)
                    if e2 == len(body) and not (e2 > c2 + 1 and False):
                        # no else on the last if
                        k = stmt_end(body, c2 + 1)
                        if k == len(body):
                            s = body[:last]
                            b = body[last + 2:c2]
                            t = body[c2 + 1:]
                            one = len(starts) == 2 and s[0] != '{'
                            out += ['if', '('] + g + [')'] + (s if one else ['{'] + s + ['}'])
                            out += ['if', '(', '('] + g + [')', '&&', '('] + b + [')', ')'] + t
                            i = e + 1
                            continue
        out.append(toks[i])
        i += 1
    return out


def main():
    a = sys.argv[1:]
    rev = 'HEAD'
    if '--rev' in a:
        k = a.index('--rev')
        rev = a[k + 1]
        del a[k:k + 2]
    path, fn, guards = a[0], a[1], [tokens(g) for g in a[2:]]
    old = subprocess.run(['git', 'show', '%s:%s' % (rev, path)], capture_output=True, text=True, check=True).stdout
    new = open(path).read()
    o = tokens(function_text(old, fn))
    n = tokens(function_text(new, fn))
    exp = rewrite(o, guards)
    if exp == o:
        print("FAIL: no guard of the old function matched a Recipe SG body")
        sys.exit(1)
    if exp == n:
        print("OK: the new %s is the old one with Recipe SG applied to %d guard site(s)" % (fn, sum(1 for _ in range(1))) if False else "OK: the new %s is Recipe SG applied to the old one" % fn)
        return
    for k, (x, y) in enumerate(zip(exp, n)):
        if x != y:
            print("FAIL at token %d: expected %r, found %r (context: %s)" % (k, x, y, ' '.join(n[max(0, k - 8):k + 8])))
            sys.exit(1)
    print("FAIL: lengths differ (%d expected, %d found)" % (len(exp), len(n)))
    sys.exit(1)


if __name__ == '__main__':
    main()
