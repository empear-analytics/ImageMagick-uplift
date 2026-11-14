#!/usr/bin/env python3
"""Check a Recipe KO step: keyed offset label groups replaced by a table of bases.

  keyed_offset_check.py FILE FUNCTION TABLE [--rev REV | --old OLDFILE]

FILE is compared as it is in the working tree against its text at REV (default HEAD) or
against OLDFILE. FUNCTION is the function whose `switch` the step replaced, TABLE the
`static const` array it added. The script reads the old `switch (E)`: groups of labels
`case L1: ... case Ln:` followed by one statement `V=(K-C);` and `break;` (braces optional),
then `default: break;`. It prints OK when all of these hold:

  - the new TABLE holds one row `{ L, C }` for every label of the old `switch`, in the
    `switch`'s order, `C` the constant of the label's group (no label lost, added, doubled or
    given another group's base);
  - the new function is `const T key = E;`, `size_t i;` and one `for (i=0; i < (sizeof(TABLE)/
    sizeof(*TABLE)); i++) if (TABLE[i].key == key) { V=(K-TABLE[i].base); break; }`, with the
    same `E`, `V` and `K` as every old group (the same lvalue and operand in every group);
  - the typedef directly above the table has the two members `key` and `base`, and the table
    is an array of it;
  - everything else of the file is the same token stream once the old function and the new
    typedef, table and function are cut out.

It cannot check the types of `key` and `base` against `E` and the constants, or that nothing
in the `switch` wrote `E` or `K`: those stay checks by reading (Recipe KO steps 1 and 3).
"""
import re
import subprocess
import sys
from collections import namedtuple

TOKEN = re.compile(
    r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|\d[\w.]*|->|<<=|>>=|\+\+|--|'
    r'<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^]=|\S')

Switch = namedtuple("Switch", "expr target operand rows")


def tokens(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    return TOKEN.findall(text)


def function_span(text, name):
    m = re.search(r"^static [^;{]*\b" + re.escape(name) + r"\([^;{]*\)\s*\n\{", text, re.M)
    if not m:
        raise SystemExit(f"FAIL: function {name} not found")
    return m.start(), text.index("\n}\n", m.end()) + 3


def controlling_expression(toks):
    """The tokens of `switch (E)` and the index of the brace that opens its body."""
    i = toks.index("switch")
    depth, j = 0, i + 1
    while True:
        depth += toks[j] == "("
        depth -= toks[j] == ")"
        if depth == 0:
            return toks[i + 2:j], j + 1
        j += 1


def need(ok, message):
    if not ok:
        raise SystemExit("FAIL: " + message)


def read_labels(toks, k):
    labels = []
    while toks[k] == "case":
        need(toks[k + 2] == ":", "a label is not `case L:`")
        labels.append(toks[k + 1])
        k += 3
    need(labels, "a group has no label")
    return labels, k


def read_statement(toks, k):
    """The one statement of a group, its `break;` and the optional braces; the index after them."""
    braced = toks[k] == "{"
    k += braced
    end = toks.index(";", k)
    stmt = toks[k:end + 1]
    need(toks[end + 1:end + 3] == ["break", ";"], "a group does not end in break;")
    k = end + 3
    need(not braced or toks[k] == "}", "a braced group holds more than one statement")
    return stmt, k + braced


def split_assignment(stmt):
    """(V, K, C) of `V=(K-C);`."""
    m = re.fullmatch(r"(.+?) = \( (.+) - (\S+) \) ;", " ".join(stmt))
    need(m, "a group is not V=(K-C); with a one-token constant")
    return m.group(1).split(), m.group(2).split(), m.group(3)


def old_switch(toks):
    expr, k = controlling_expression(toks)
    need(toks[k] == "{", "the switch body does not open with a brace")
    k += 1
    rows, shapes = [], set()
    while toks[k] != "default":
        labels, k = read_labels(toks, k)
        stmt, k = read_statement(toks, k)
        v, operand, c = split_assignment(stmt)
        shapes.add((tuple(v), tuple(operand)))
        rows += [(label, c) for label in labels]
    need(len(shapes) == 1, "the groups do not share one V and one K")
    need(toks[k:k + 5] == ["default", ":", "break", ";", "}"], "default is not `default: break;` at the end")
    target, operand = next(iter(shapes))
    return Switch(expr, list(target), list(operand), rows)


def table_rows(toks, table):
    starts = [n for n, t in enumerate(toks) if t == table and toks[n + 1:n + 5] == ["[", "]", "=", "{"]]
    if not starts:
        return None
    end = toks.index(";", starts[-1])
    inner = " ".join(toks[starts[-1] + 5:end - 1])
    rest = re.sub(r"\{ \S+ , \S+ \}( ,)?", "", inner).strip()
    need(rest == "", "a table row is not { key, base }")
    return re.findall(r"\{ (\S+) , (\S+) \}", inner)


def check_table(old_rows, new_toks, table):
    rows = table_rows(new_toks, table)
    problems = []
    if len({label for label, _ in old_rows}) != len(old_rows):
        problems.append("the old switch holds a label twice")
    if rows is None:
        problems.append(f"table {table} not found above the function")
    elif rows != old_rows:
        problems.append(f"table rows {rows} are not the old labels with their bases {old_rows}")
    return problems


def find_typedef(text, table):
    return re.search(r"typedef struct _\w+\s*\{([^{}]*)\}\s*(\w+)\s*;\s*static const (\w+)\s+" +
                     re.escape(table) + r"\b", text)


def check_typedef(typedef):
    if not typedef:
        return ["no typedef directly above the table"]
    problems = []
    names = [t for t in tokens(typedef.group(1)) if t not in (",", ";")]
    if len(names) != 3 or names[1:] != ["key", "base"]:
        problems.append("the typedef does not hold exactly the members key and base")
    if typedef.group(2) != typedef.group(3):
        problems.append("the table is not an array of the typedef")
    return problems


def expected_loop(sw, table):
    head = ["for", "(", "i", "=", "0", ";", "i", "<", "(", "sizeof", "(", table, ")", "/", "sizeof", "(", "*",
            table, ")", ")", ";", "i", "++", ")", "if", "(", table, "[", "i", "]", ".", "key", "==", "key", ")",
            "{"]
    store = sw.target + ["=", "("] + sw.operand + ["-", table, "[", "i", "]", ".", "base", ")", ";"]
    return head + store + ["break", ";", "}"]


def check_function(sw, new_text, span, table):
    body = tokens(new_text[span[0]:span[1]])
    inner = body[body.index("{") + 1:-1]
    if inner[:1] != ["const"] or inner[2:4] != ["key", "="]:
        return ["the body does not open with `const T key = E;`"]
    head = ["const", inner[1], "key", "="] + sw.expr + [";", "size_t", "i", ";"]
    if inner != head + expected_loop(sw, table):
        return ["the new function is not the declarations and the loop of Recipe KO step 4"]
    return []


def parse_args(argv):
    args = list(argv)
    options = {"--rev": "HEAD", "--old": None}
    for flag in options:
        if flag in args:
            i = args.index(flag)
            options[flag] = args[i + 1]
            del args[i:i + 2]
    if len(args) != 3:
        raise SystemExit(__doc__)
    return args, options


def read_texts(path, options):
    with open(path, encoding="utf-8", errors="surrogateescape") as f:
        new = f.read()
    if options["--old"]:
        with open(options["--old"], encoding="utf-8", errors="surrogateescape") as f:
            return f.read(), new
    old = subprocess.run(["git", "show", f"{options['--rev']}:{path}"], capture_output=True, text=True,
                         check=True, errors="surrogateescape").stdout
    return old, new


def main(argv):
    (path, func, table), options = parse_args(argv)
    old, new = read_texts(path, options)
    o_span, n_span = function_span(old, func), function_span(new, func)
    sw = old_switch(tokens(old[o_span[0]:o_span[1]]))
    typedef = find_typedef(new[:n_span[0]], table)
    problems = check_table(sw.rows, tokens(new[:n_span[0]]), table)
    problems += check_typedef(typedef)
    problems += check_function(sw, new, n_span, table)
    cut_from = typedef.start() if typedef else n_span[0]
    if tokens(old[:o_span[0]] + old[o_span[1]:]) != tokens(new[:cut_from] + new[n_span[1]:]):
        problems.append("the file differs outside the function and the new typedef and table")
    for problem in problems:
        print("FAIL:", problem)
    if problems:
        return 1
    groups = len({c for _, c in sw.rows})
    print(f"OK {func}: {len(sw.rows)} labels in {groups} groups, table {table}, one loop, "
          f"the rest of the file unchanged")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
