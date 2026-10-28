#!/usr/bin/env python3
"""Recipe E for one case of a function's closing `switch`.

    tools/extract_case.py FILE FUNCTION LABEL NEW_NAME [--dry-run]

Moves the body of `case LABEL:` (or `default:`) of the `switch` that ends
FUNCTION into a new static function NEW_NAME, directly above FUNCTION, and
leaves a call followed by `break;` in its place. A case that is only
`if (C) { ... break; }` and falls through keeps its test: the block's
contents move instead.

The move is legal only when the moved code sees exactly the values the
function's locals had on entry, so the script refuses unless:

- FUNCTION's body is its declarations, `assert` lines and guard clauses that
  only report and return, then the `switch`, which is its last statement;
- every case falling into LABEL is such a test, its condition free of
  assignments and calls, so the path that falls through writes nothing;
- LABEL does not fall through, except past a test as above;
- every `break;` that leaves the case is outside any loop or inner `switch`
  (those become `return;`, and the case's last one is dropped).

The new function takes the parameters, and declares the locals, that the
moved code uses, locals with their initialisers (copied from FUNCTION);
FUNCTION drops the locals it no longer uses.
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


ASSIGNMENT = re.compile(r"[^=!<>]=[^=]|\+\+|--")


def guard_length(lines, i):
    """Length of an `if` block at I that ends in `return;` and assigns
    nothing, or 0."""
    if not lines[i].startswith("  if ("):
        return 0
    brace = lines.index("    {\n", i)
    close = lines.index("    }\n", brace)
    block = "".join(lines[i:close])
    if lines[close - 1] != "      return;\n" or ASSIGNMENT.search(block):
        return 0
    return close + 1 - i


def prologue_entry(lines, i):
    """(declaration group or [], lines taken) for the line at I: a
    declaration, an assert, a blank line or a guard clause."""
    if lines[i].startswith("  assert(") or lines[i] == "\n":
        return [], 1
    taken = guard_length(lines, i)
    if taken:
        return [], taken
    if is_declaration(lines, i):
        group = read_declaration(lines, i)
        return group, len(group)
    raise Refused("statement before the switch: %s" % lines[i].strip())


def split_prologue(lines, start, end):
    """Signature, declaration groups and the switch's line range."""
    brace = lines.index("{\n", start)
    declarations = []
    i = brace + 1
    while lines[i] != "  switch (quantum_info->depth)\n":
        entry, taken = prologue_entry(lines, i)
        if entry:
            declarations.append(entry)
        i += taken
    if lines[end - 1] != "  }\n":
        raise Refused("the switch is not the last statement")
    return lines[start:brace], declarations, (i, end - 1)


CALL = re.compile(r"\b(?!if\b)[A-Za-z_]\w*\s*\(")


def opener_of(label):
    if label == "default":
        return "    default:\n"
    return "    case %s:\n" % label


def guard_branch(body):
    """For a case body that is `if (C) { ... break; }` followed by
    magick_fallthrough, with C free of assignments and calls, the line
    range of the block's contents (without its break); otherwise None."""
    if body[-3:] != ["          break;\n", "        }\n",
                     "      magick_fallthrough;\n"]:
        return None
    if not body[0].startswith("      if ("):
        return None
    brace = body.index("        {\n")
    condition = "".join(body[:brace]).strip()[2:]
    if ASSIGNMENT.search(condition) or CALL.search(condition):
        return None
    if body.index("        }\n", brace) != len(body) - 2:
        return None
    return brace + 1, len(body) - 3


def previous_case(lines, at):
    """Opening line of the case before the one at AT."""
    i = at - 2
    while not (lines[i].startswith("    case ") and lines[i + 1] == "    {\n"):
        i -= 1
    return i


def falls_in_cleanly(lines, at):
    """Every case that falls into the one at AT only tests and breaks."""
    while lines[at - 2] == "      magick_fallthrough;\n":
        before = previous_case(lines, at)
        if guard_branch(lines[before + 2:at - 1]) is None:
            raise Refused("a case falling into line %d writes" % (at + 1))
        at = before
    return True


def case_checks(lines, at, close, label):
    """(failed, reason) pairs for the case whose braces end at CLOSE."""
    return [
        (lines[at - 1].strip() not in ("}", "{"),
         "case %s shares its body with another label" % label),
        (lines[at + 1] != "    {\n", "case %s has no braces" % label),
        (not falls_in_cleanly(lines, at), "unreachable"),
    ]


def movable_part(lines, at, close, label):
    """(first line, end line, indent) of what moves out of the case."""
    branch = guard_branch(lines[at + 2:close])
    if branch:
        return at + 2 + branch[0], at + 2 + branch[1], 10
    if "magick_fallthrough" in "".join(lines[at:close]):
        raise Refused("case %s falls through" % label)
    return at + 2, close, 6


def find_case(lines, first, last, label):
    """Line range of the part of `case LABEL:` to move: the whole body,
    or the guarded block of a case that tests and falls through."""
    opener = opener_of(label)
    if opener not in lines[first:last]:
        raise Refused("no case %s" % label)
    at = lines.index(opener, first)
    close = lines.index("    }\n", at + 1)
    reasons = [reason for failed, reason in case_checks(lines, at, close, label)
               if failed]
    if reasons:
        raise Refused(reasons[0])
    return movable_part(lines, at, close, label)


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


def drop_final_return(out, indent):
    """The case's own last break ends the new function anyway."""
    while out and out[-1].strip() == "":
        out.pop()
    last = out[-1] if out else ""
    if last == " " * indent + "return;\n":
        out.pop()
    return out


def convert_breaks(body, indent):
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
    return drop_final_return(out, indent)


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


def parameters(signature):
    """(declaration, name) for each parameter of SIGNATURE."""
    text = " ".join(line.strip() for line in signature)
    text = text[text.index("(") + 1:text.rindex(")")]
    return [(param.strip(), IDENT.findall(param)[-1])
            for param in text.split(",")]


def format_signature(name, declarations):
    """ImageMagick's layout: wrapped at 80 columns, continued by two
    spaces."""
    lines, line = [], "static void %s(" % name
    for index, declaration in enumerate(declarations):
        last = index == len(declarations) - 1
        piece = declaration + (")" if last else ",")
        if len(line) + len(piece) > 80:
            lines.append(line + "\n")
            line = "  "
        line += piece
    return lines + [line + "\n"]


def uses_group(text, group):
    return any(uses(text, name) for name in declared_names(group))


def build_helper(new_name, signature, declarations, body):
    """The new function's lines and the parameter names to call it with."""
    body_text = "".join(body)
    helper_decls = [g for g in declarations if uses_group(body_text, g)]
    used_text = body_text + "".join(sum(helper_decls, []))
    params = [p for p in parameters(signature) if uses(used_text, p[1])]
    helper = format_signature(new_name, [p[0] for p in params]) + ["{\n"]
    for group in helper_decls:
        helper += group + ["\n"]
    return helper + body + ["}\n", "\n"], [p[1] for p in params]


def drop_unused(rest, skip, declarations):
    """Remove the declaration groups REST no longer uses after line SKIP."""
    for group in declarations:
        remaining = "".join(rest[skip:]).replace("".join(group), "")
        if not uses_group(remaining, group):
            index = find_run(rest, group)
            del rest[index:index + len(group) + 1]
    return rest


def extract(lines, function, label, new_name):
    start, end = find_function(lines, function)
    signature, declarations, (first, last) = split_prologue(lines, start, end)
    first_line, close, indent = find_case(lines, first, last, label)
    moved = lines[first_line:close]
    body = [line[indent - 2:] if line.strip() else line
            for line in convert_breaks(moved, indent)]
    helper, names = build_helper(new_name, signature, declarations, body)
    pad = " " * indent
    call = "%s%s(%s);\n" % (pad, new_name, ",".join(names))
    rest = (lines[start:first_line] + [call, pad + "break;\n"] +
            lines[close + (indent == 10):end + 1])
    rest = drop_unused(rest, len(signature), declarations)
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
