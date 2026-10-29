#!/usr/bin/env python3
"""Recipe E for a guarded block that ends its function.

    tools/extract_block.py FILE FUNCTION N NEW_NAME [--dry-run]

NEW_NAME may hold `{tag}`, replaced by a word for the block's test (Float,
Unpacked, MinIsWhite, ...; Integer, Packed, ... for the `else` of one); a
test without a word is refused. Blocks already reduced to one call and
`return;` are skipped and not counted.

In a static void function whose body is declarations, then single-line
assignments to locals and top-level `if (C)` / `else` blocks, the N-th such
block whose last statement is `return;` moves into a new static function
NEW_NAME, directly above FUNCTION; the block keeps its test and becomes the
call followed by `return;`.

The move is legal only when the moved block sees the values it saw before,
so the script refuses unless everything between the declarations and the
block is either an assignment `local=...;` (that local is passed to the new
function as a parameter, step 3 of Recipe E) or a block that ends in
`return;` under a condition free of assignments and calls (the path that
skips it writes nothing). Locals the block uses that nothing before it
assigns are declared in the new function as they were, initialisers
included; the block's own writes to them are not read after it, since the
block ends the function (Recipe E steps 4 and 7).
"""

import argparse
import re
import sys

from extract_case import (ASSIGNMENT, CALL, Refused, Source, build_helper,
                          check_new_name, declared_names, drop_unused,
                          find_function,
                          format_call, is_declaration, parameters,
                          read_declaration, uses)

ASSIGNED = re.compile(r"  (\w+)=[^=].*;\n$")


def read_block(lines, i):
    """(header lines, contents, end) of the top-level block at I."""
    brace = lines.index("    {\n", i)
    close = lines.index("    }\n", brace)
    return lines[i:brace], lines[brace + 1:close], close + 1


def is_clean_test(header):
    condition = "".join(header).strip()
    if condition == "else":
        return True
    condition = condition[2:]
    return not (ASSIGNMENT.search(condition) or CALL.search(condition))


def ends_function(contents):
    return contents[-1:] == ["      return;\n"]


def is_moved(contents):
    code = [line for line in contents if line.strip()]
    return len(code) == 2 and ends_function(contents)


TAGS = {
    "quantum_info->format == FloatingPointQuantumFormat": ("Float", "Integer"),
    "quantum_info->pack == MagickFalse": ("Unpacked", "Packed"),
    "quantum_info->min_is_white != MagickFalse": ("MinIsWhite", "MinIsBlack"),
    "quantum_info->quantum == 32U": ("Long", "Short"),
    "quantum_info->quantum == 32UL": ("Long", "Short"),
    "quantum_info->endian == LSBEndian": ("LSB", "MSB"),
}


def tag_of(header, previous):
    """The naming word for the block under HEADER; PREVIOUS is the
    condition of the `if` an `else` belongs to."""
    condition = "".join(header).strip()
    negated = condition == "else"
    if negated:
        condition = previous
    condition = condition[4:-1] if condition.startswith("if (") else condition
    if condition not in TAGS:
        raise Refused("no name for the test %s" % condition)
    return TAGS[condition][1 if negated else 0]


STATEMENT = re.compile(r"  (if|else|for|while|switch|do)\b|  \w+=")


def starts_declaration(lines, i):
    return not STATEMENT.match(lines[i]) and is_declaration(lines, i)


def skip_declarations(lines, brace):
    """Declaration groups after the brace, and the first line after them."""
    groups, i = [], brace + 1
    while lines[i] == "\n" or starts_declaration(lines, i):
        if lines[i] == "\n":
            i += 1
            continue
        groups.append(read_declaration(lines, i))
        i += len(groups[-1])
    return groups, i


def check_header(lines, i):
    if not lines[i].startswith(("  if (", "  else")):
        raise Refused("statement before the block: %s" % lines[i].strip())


def is_target(contents, ordinal):
    """(is a block still to move, ordinal left after counting it)."""
    candidate = ends_function(contents) and not is_moved(contents)
    return candidate, ordinal - candidate


def check_skipped(header, contents, i):
    if not (ends_function(contents) and is_clean_test(header)):
        raise Refused("block at line %d may write and fall through" % i)


def scan(lines, i, ordinal):
    """The ORDINAL-th returning block from line I (header range, contents,
    naming tag), and the locals assigned before it."""
    assigned, previous = [], ""
    while not lines[i].startswith("}"):
        match = ASSIGNED.match(lines[i])
        if match:
            assigned.append(match.group(1))
            i += 1
            continue
        check_header(lines, i)
        header, contents, end = read_block(lines, i)
        candidate, ordinal = is_target(contents, ordinal)
        if candidate and ordinal == 0:
            tag = tag_of(header, previous)
            return (i, len(header), end), contents, assigned, tag
        check_skipped(header, contents, i)
        previous = "".join(header).strip()
        i = end
    raise Refused("no such block")


def as_parameter(groups, name):
    """`type name` for a local declared alone and without initialiser."""
    group = [g for g in groups if name in declared_names(g)][0]
    if group[1:] != ["    %s;\n" % name]:
        raise Refused("%s is not declared alone, uninitialised" % name)
    return "%s %s" % (group[0].strip(), name)


def helper_source(signature, groups, assigned):
    """The new function's view: FUNCTION's parameters plus the assigned
    locals, and the other locals' declarations."""
    params = [p[0] for p in parameters(signature)]
    params += [as_parameter(groups, name) for name in assigned]
    synthetic = ["static void F(%s)\n" % ",".join(params)]
    kept = [g for g in groups if not set(declared_names(g)) & set(assigned)]
    return Source(synthetic, kept)


def extract(lines, function, ordinal, new_name):
    check_new_name(lines, new_name)
    start, end = find_function(lines, function)
    brace = lines.index("{\n", start)
    groups, first = skip_declarations(lines, brace)
    (at, header_length, block_end), contents, assigned, tag = scan(
        lines, first, ordinal)
    new_name = new_name.replace("{tag}", tag)
    check_new_name(lines, new_name)
    body = [line[4:] if line.strip() else line for line in contents[:-1]]
    assigned = [name for name in assigned if uses("".join(body), name)]
    source = helper_source(lines[start:brace], groups, assigned)
    helper, names = build_helper(new_name, source, body)
    call = format_call("      ", "", new_name, names)
    block = (lines[at:at + header_length] + ["    {\n"] + call +
             ["      return;\n", "    }\n"])
    rest = lines[start:at] + block + lines[block_end:end + 1]
    rest = drop_unused(rest, brace + 1 - start, groups)
    return lines[:start] + helper + rest + lines[end + 1:]


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("file")
    parser.add_argument("function")
    parser.add_argument("ordinal", type=int)
    parser.add_argument("new_name")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    with open(args.file) as handle:
        lines = handle.readlines()
    try:
        result = extract(lines, args.function, args.ordinal, args.new_name)
    except Refused as reason:
        print("REFUSED: %s" % reason)
        return 1
    if args.dry_run:
        sys.stdout.writelines(result)
        return 0
    with open(args.file, "w") as handle:
        handle.writelines(result)
    print("extracted block %d of %s" % (args.ordinal, args.function))
    return 0


if __name__ == "__main__":
    sys.exit(main())
