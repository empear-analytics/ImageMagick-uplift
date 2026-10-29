#!/usr/bin/env python3
"""Recipe E for one case of a switch that follows computed locals.

    tools/extract_switch_case.py FILE FUNCTION SWITCH LABEL NEW_NAME [--dry-run]

SWITCH is the switch's expression as written (`stream_info->storage_type`);
the switch must be a statement of FUNCTION's body, at the top level. The
body of `case LABEL:` (or the guarded block of a case that tests and falls
through, as in tools/extract_case.py) moves into a new static void function
NEW_NAME directly above FUNCTION; a call and `break;` take its place.

Unlike tools/extract_case.py, statements may precede the switch and follow
it. What the moved code needs is settled by these rules:

- a local assigned before the switch and read by the moved code is passed
  by value (Recipe E step 3); a write to it inside the case stays in the new
  function's copy;
- a local the case uses that nothing before the switch assigns is declared
  in the new function as it was, initialiser included;
- a loop index used only inside `for` loops that initialise it is declared
  afresh, not passed;
- the move is refused when the case writes a local read after the switch
  (step 4: only variables nothing reads afterwards may be written), or holds
  a `return`.
"""

import argparse
import re
import sys

from extract_block import skip_declarations
from extract_case import (Refused, Source, build_helper, check_new_name,
                          check_not_moved, convert_breaks, declared_names,
                          drop_unused, find_case, format_call, parameters,
                          uses)


def find_any_function(lines, name):
    head = re.compile(r"static \w+ %s\(" % re.escape(name))
    for start, line in enumerate(lines):
        if head.match(line):
            return start, lines.index("}\n", start)
    raise Refused("no static function %s" % name)


def writes(text, name):
    """Whether TEXT assigns, increments or takes the address of NAME."""
    n = re.escape(name)
    pattern = (r"(?<![\w>.])%s\s*(?:[-+*/%%&|^]|<<|>>)?=(?!=)|"
               r"(?:\+\+|--)\s*%s\b|(?<![\w>.])%s\s*(?:\+\+|--)|&\s*%s\b"
               % (n, n, n, n))
    return re.search(pattern, text) is not None


def loop_spans(moved, name):
    """Line ranges of the `for (NAME=...` loops in MOVED, braces included."""
    spans = []
    for i, line in enumerate(moved):
        if re.match(r"\s*for \(%s=" % re.escape(name), line):
            indent = len(line) - len(line.lstrip())
            close = moved.index(" " * indent + "}\n", i)
            spans.append((i, close + 1))
    return spans


def is_loop_index(moved, name):
    """NAME occurs only inside loops whose header initialises it."""
    spans = loop_spans(moved, name)
    used = [i for i, line in enumerate(moved) if uses(line, name)]
    return bool(spans) and all(any(a <= i < b for a, b in spans)
                               for i in used)


def declarator(group, name):
    """`type declarator` of NAME in its declaration GROUP, initialiser and
    trailing punctuation removed."""
    line = [l for l in group[1:] if name in declared_names([group[0], l])][0]
    text = line.strip().rstrip(",;").split(" = ")[0]
    return "%s %s" % (group[0].strip(), text)


def local_names(groups):
    return [(g, n) for g in groups for n in declared_names(g)]


def passed(groups, prologue, moved):
    """Locals assigned before the switch that the moved case reads."""
    text = "".join(moved)
    return [n for _, n in local_names(groups)
            if writes(prologue, n) and uses(text, n)
            and not is_loop_index(moved, n)]


def check_clash(groups, text, tail):
    clash = [n for _, n in local_names(groups)
             if writes(text, n) and uses(tail, n)]
    if clash:
        raise Refused("the case writes %s, read after the switch" % clash[0])


def plan(groups, prologue, moved, tail):
    """(parameters to add, declaration groups to keep) for the moved case."""
    assigned = passed(groups, prologue, moved)
    check_clash(groups, "".join(moved), tail)
    extra = [declarator(g, n) for g, n in local_names(groups)
             if n in assigned]
    return extra, [split_group(g, assigned) for g in groups
                   if set(declared_names(g)) - set(assigned)]


def split_group(group, assigned):
    """GROUP without the names that become parameters."""
    kept = [l for l in group[1:]
            if not set(declared_names([group[0], l])) & set(assigned)]
    if len(kept) == len(group) - 1:
        return group
    kept[-1] = kept[-1].rstrip().rstrip(",;") + ";\n"
    return [group[0]] + kept


def locate(lines, start, end, switch):
    opener = "  switch (%s)\n" % switch
    if lines[start:end].count(opener) != 1:
        raise Refused("no single top-level switch (%s)" % switch)
    first = lines.index(opener, start)
    return first, lines.index("  }\n", first)


def extract(lines, request):
    function, switch, label, new_name = request
    check_new_name(lines, new_name)
    start, end = find_any_function(lines, function)
    brace = lines.index("{\n", start)
    groups, body_start = skip_declarations(lines, brace)
    first, last = locate(lines, start, end, switch)
    first_line, close, indent = find_case(lines, first, last, label)
    moved = lines[first_line:close]
    check_not_moved(moved)
    if any(re.search(r"\breturn\b", line) for line in moved):
        raise Refused("case %s returns from %s" % (label, function))
    extra, kept = plan(groups, "".join(lines[body_start:first]), moved,
                       "".join(lines[last + 1:end]))
    params = [p[0] for p in parameters(lines[start:brace])] + extra
    source = Source(["static void F(%s)\n" % ",".join(params)], kept)
    body = [line[indent - 2:] if line.strip() else line
            for line in convert_breaks(moved, indent)]
    helper, names = build_helper(new_name, source, body)
    pad = " " * indent
    rest = (lines[start:first_line] + format_call(pad, "", new_name, names) +
            [pad + "break;\n"] + lines[close + (indent == 10):end + 1])
    rest = drop_unused(rest, brace + 1 - start, groups)
    return lines[:start] + helper + rest + lines[end + 1:]


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    for name in ("file", "function", "switch", "label", "new_name"):
        parser.add_argument(name)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    with open(args.file) as handle:
        lines = handle.readlines()
    try:
        result = extract(lines, (args.function, args.switch, args.label,
                                 args.new_name))
    except Refused as reason:
        print("REFUSED: %s" % reason)
        return 1
    if args.dry_run:
        sys.stdout.writelines(result)
        return 0
    with open(args.file, "w") as handle:
        handle.writelines(result)
    print("extracted case %s of %s into %s" % (args.label, args.function,
                                               args.new_name))
    return 0


if __name__ == "__main__":
    sys.exit(main())
