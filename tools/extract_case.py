#!/usr/bin/env python3
"""Recipe E for one case of a function's closing `switch`.

    tools/extract_case.py FILE FUNCTION LABEL NEW_NAME [--dry-run]

Moves the body of `case LABEL:` of the `switch` that ends FUNCTION into a new
static function NEW_NAME, directly above FUNCTION, with FUNCTION's own
parameters, and leaves a call followed by `break;` in its place.

The move is legal only when the case body sees exactly the values the
function's locals had on entry, so the script refuses unless:

- FUNCTION's body is its declarations, then `assert` lines only, then the
  `switch`, which is its last statement;
- no case falls through into LABEL, and LABEL does not fall through;
- every `break;` that leaves the case is outside any loop or inner `switch`
  (those become `return;`, and the case's last one is dropped).

The new function declares the locals the case body uses, with their
initialisers (copied from FUNCTION); FUNCTION drops those it no longer uses.
"""

import argparse
import re
import sys

IDENT = re.compile(r"[A-Za-z_]\w*")
LOOP = re.compile(r"(for|while|switch|do)\b")
COND = re.compile(r"(if|else)\b")


class Refused(Exception):
    pass


def find_function(lines, name):
    """Index of the signature line and of the closing brace."""
    head = re.compile(r"static void %s\(" % re.escape(name))
    for start, line in enumerate(lines):
        if head.match(line):
            end = lines.index("}\n", start)
            return start, end
    raise Refused("no static void function %s" % name)


def is_declaration(lines, i):
    """A type line followed by an indented name line."""
    return not lines[i].startswith("    ") and lines[i + 1].startswith("    ")


def read_declaration(lines, i):
    """The lines of the declaration group starting at I."""
    j = i + 1
    while not lines[j - 1].rstrip().endswith(";"):
        j += 1
    return lines[i:j]


def prologue_entry(lines, i):
    """The declaration group at I, [] for an assert or a blank line."""
    if lines[i].startswith("  assert(") or lines[i] == "\n":
        return []
    if is_declaration(lines, i):
        return read_declaration(lines, i)
    raise Refused("statement before the switch: %s" % lines[i].strip())


def split_prologue(lines, start, end):
    """Signature, declaration groups and the switch's line range."""
    brace = lines.index("{\n", start)
    declarations = []
    i = brace + 1
    while lines[i] != "  switch (quantum_info->depth)\n":
        entry = prologue_entry(lines, i)
        if entry:
            declarations.append(entry)
        i += max(len(entry), 1)
    if lines[end - 1] != "  }\n":
        raise Refused("the switch is not the last statement")
    return lines[start:brace], declarations, (i, end - 1)


def case_checks(lines, at, close, label):
    """(failed, reason) pairs for the case whose braces end at CLOSE."""
    inside = "".join(lines[at:close])
    return [
        (lines[at - 1].strip() not in ("}", "{"),
         "case %s shares its body with another label" % label),
        (lines[at - 2].strip() == "magick_fallthrough;",
         "the case before %s falls through into it" % label),
        (lines[at + 1] != "    {\n", "case %s has no braces" % label),
        ("magick_fallthrough" in inside, "case %s falls through" % label),
    ]


def find_case(lines, first, last, label):
    """Line range of `case LABEL:`'s braces, with a fallthrough check."""
    opener = "    case %s:\n" % label
    if opener not in lines[first:last]:
        raise Refused("no case %s" % label)
    at = lines.index(opener, first)
    close = lines.index("    }\n", at + 1)
    reasons = [reason for failed, reason in case_checks(lines, at, close, label)
               if failed]
    if reasons:
        raise Refused(reasons[0])
    return at, close


def header_kind(text, pending):
    """What a line leaves pending for the brace or statement after it."""
    if LOOP.match(text):
        return "loop"
    if COND.match(text):
        return "cond"
    if text.endswith(";"):
        return None
    return pending


def convert_break(line, stack, pending):
    """A break that leaves the case becomes a return."""
    if "loop" in stack:
        return line
    if pending == "loop":
        raise Refused("brace-less loop with a break")
    return line.replace("break;", "return;")


def drop_final_return(out):
    """The case's own last break ends the new function anyway."""
    while out and out[-1].strip() == "":
        out.pop()
    last = out[-1] if out else ""
    if last == "      return;\n":
        out.pop()
    return out


def convert_breaks(body):
    """Turn the case-level breaks into returns; refuse ambiguous ones."""
    stack, pending, out = [], None, []
    for line in body:
        text = line.strip()
        if text == "{":
            stack.append(pending or "block")
            pending = None
        elif text == "}":
            stack.pop()
        elif text == "break;":
            line = convert_break(line, stack, pending)
            pending = None
        else:
            pending = header_kind(text, pending)
        out.append(line)
    return drop_final_return(out)


def uses(text, name):
    return re.search(r"\b%s\b" % re.escape(name), text) is not None


def declared_names(group):
    names = []
    for line in group[1:]:
        match = IDENT.search(line.replace("magick_restrict", "").replace("*", ""))
        if match:
            names.append(match.group(0))
    return names


def find_run(lines, run):
    """Index where the lines of RUN occur together in LINES."""
    for index in range(len(lines) - len(run) + 1):
        if lines[index:index + len(run)] == run:
            return index
    raise Refused("declaration not found")


def parameter_names(signature):
    text = "".join(signature)
    params = text[text.index("(") + 1:text.rindex(")")]
    return [IDENT.findall(param)[-1] for param in params.split(",")]


def extract(lines, function, label, new_name):
    start, end = find_function(lines, function)
    signature, declarations, (first, last) = split_prologue(lines, start, end)
    at, close = find_case(lines, first, last, label)
    body = [line[4:] if line.strip() else line
            for line in convert_breaks(lines[at + 2:close])]
    body_text = "".join(body)
    helper_decls = [group for group in declarations
                    if any(uses(body_text, n) for n in declared_names(group))]
    helper = [signature[0].replace(function, new_name, 1)]
    helper += signature[1:] + ["{\n"]
    for group in helper_decls:
        helper += group + ["\n"]
    helper += body + ["}\n", "\n"]
    call = "      %s(%s);\n" % (new_name, ",".join(parameter_names(signature)))
    rest = lines[start:at + 2] + [call, "      break;\n"] + lines[close:end + 1]
    rest_text = "".join(rest[len(signature):])
    for group in declarations:
        if not any(uses(rest_text.replace("".join(group), ""), n)
                   for n in declared_names(group)):
            index = find_run(rest, group)
            del rest[index:index + len(group) + 1]
    return lines[:start] + helper + rest + lines[end + 1:]


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("file")
    parser.add_argument("function")
    parser.add_argument("label")
    parser.add_argument("new_name")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    with open(args.file) as handle:
        lines = handle.readlines()
    try:
        result = extract(lines, args.function, args.label, args.new_name)
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
