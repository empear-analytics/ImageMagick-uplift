#!/usr/bin/env python3
"""Check the targets, the input fill, the bindings and the rewrite of a Recipe CT step.

  case_tail_check.py FILE TARGETS HELPER [INPUT LOCAL] [--returns] [--shared]

With `--shared` the INPUT local is the shared struct of Recipe N step 8: its fill is one run
of `LOCAL.m=m;` anywhere before the calls, not directly before this helper's call.

With `--returns` the helper is the `static MagickBooleanType` function of Recipe CR (a
tail that ends its case or iteration or returns failure): each `break;` or `continue;`
the block lost is read as `return(MagickTrue);`, each `return(c);` as
`return(MagickFalse);`, the helper ends with one more `return(MagickTrue);`, and the
call is `if (HELPER(...) == MagickFalse)` followed by `return(c);` and the case's
`break;` (or the loop's closing brace). Everything else is as below.

TARGETS is the targets typedef (members are pointers to the caller's written
variables, each named after its variable), HELPER the new `static void` function whose
last parameter is `const TARGETS *`, INPUT and LOCAL the optional input typedef and the
caller's local of that type (Recipe Y steps 3 to 5). The script checks, for the form
Recipe CT gives:

  - the targets: one `const TARGETS name = { &m1, &m2, ... };` initialiser naming every
    member, in declaration order, by the address of the variable of its name;
  - the bindings: the declarations that open HELPER's body bind `v = p->v` from the
    targets parameter `p` (each a member, under its own name; a member the block does
    not write is not bound) and `x = q->x` from the input parameter `q` (each a member
    of INPUT, under its own name);
  - the fill: one run of `LOCAL.m=m;` for every member of INPUT, in declaration order,
    directly followed by the call of HELPER that passes `&LOCAL`; no other assignment
    to a member of LOCAL;
  - the position: the statement after the call is `break;` (the case's closing
    `break;`, which stays in the caller);
  - the rewrite: every line the working tree removed from FILE against HEAD
    (`git diff`), with each bound target `v` written as step 5 writes it (`&v` becomes
    `v`, any other `v` becomes `*v`; a name after `.` or `->` is a field and stays) and
    each `break;` written `return;`, and HELPER's body lines, `(*v)` read as `*v`, are
    the same multiset, indentation and a declarator's trailing `,` or `;` aside, the
    bindings and their type lines apart. Lines the diff shows as removed and added
    unchanged outside HELPER (an alignment of the diff, not a change) cancel out;
  - a temporary (Recipe CT step 2, Recipe E step 4): a declaration in HELPER, a type
    line and `name;`, that no removed line accounts for is accepted when the removed
    lines use the name and the caller still declares it under the same type line
    (the caller keeps its declaration because it uses the name elsewhere, as a loop
    counter); whether the block writes it before reading it is checked by reading;
  - nothing else is added: every other added line outside HELPER's body belongs to a
    typedef, HELPER's signature, the targets declaration, LOCAL's declaration, its fill
    or the call.

It prints OK or FAIL with the line that broke the rule. It cannot check types, the
precedence of `*v` against `.`, `->` and `[]` (a wrong one does not compile for the
struct and scalar members Recipe CT carries, or is checked by reading), whether a
`break;` of the block ended the `switch` (a `break;` of a loop nested in the block would
stay `break;`, which this script refuses: such a block is checked by reading), nor what
the block reads from the caller beyond the fill (playbook, Recipe CT).
"""
import re
import subprocess
import sys
from collections import Counter

TRIVIAL = {"", "{", "}", "/*", "*/"}
BINDING = re.compile(r"^[\s*]*(?:const\b[\s*]*)*(\w+)\s*=\s*(\w+)->(\w+)\s*[,;]$")


def members(text, type_name):
    """Member names of `typedef struct _X { ... } TYPE;`, in declaration order."""
    m = re.search(r"typedef struct _\w+\s*\{([^{}]*)\}\s*" + re.escape(type_name) +
                  r"\s*;", text)
    if not m:
        raise SystemExit(f"FAIL: typedef {type_name} not found")
    names = []
    for decl in m.group(1).split(";"):
        for part in decl.split(","):
            words = re.findall(r"\w+", part)
            if words:
                names.append(words[-1])
    return names, m.group(0)


RETURNS = False  # Recipe CR: the helper is a `static MagickBooleanType` function


def helper_parts(text, helper):
    rtype = "MagickBooleanType" if RETURNS else "void"
    m = re.search(r"^static " + rtype + r"\s+" + re.escape(helper) +
                  r"\((.*?)\)\s*\n\{(.*?)\n\}", text, re.S | re.M)
    if not m:
        raise SystemExit(f"FAIL: static {rtype} {helper}(...) not found")
    return m.group(0), m.group(1), m.group(2)


def param_name(params, type_name):
    p = re.search(r"const\s+" + re.escape(type_name) + r"\s*\*\s*(\w+)", params)
    return p.group(1) if p else None


def check_targets(text, type_name, names):
    init = re.findall(r"const\s+" + re.escape(type_name) + r"\s+(\w+)\s*=\s*\{([^}]*)\};",
                      text)
    if len(init) != 1:
        return None, [f"expected one const {type_name} initialiser, found {len(init)}"]
    local, values = init[0][0], [v.strip() for v in init[0][1].split(",")]
    want = ["&" + n for n in names]
    if values != want:
        return local, [f"initialiser {values} is not {want}"]
    return local, []


def check_fill(text, input_type, local, helper, names):
    fill = "".join(r"\s*" + re.escape(f"{local}.{n}={n};") for n in names)
    call = r"\s*(?:if \()?" + re.escape(helper) + r"\(\s*&" + re.escape(local) + r"\b"
    found = re.findall(fill + call, text)
    problems = []
    if len(found) != 1:
        problems.append(f"expected one run of {local}.m=m; for {names} directly "
                        f"before {helper}(&{local},...), found {len(found)}")
    for m, v in re.findall(re.escape(local) + r"\.(\w+)=([^;]*);", text):
        if m != v.strip():
            problems.append(f"{local}.{m}={v};")
    return problems


def check_fill_shared(text, local, names):
    """The shared form (Recipe CT step 4, the sibling blocks of Recipe N step 8): one run of
    LOCAL.m=m; for every member, in declaration order, filled once somewhere before the
    calls (the loop); no other assignment to a member of LOCAL."""
    fill = "".join(r"\s*" + re.escape(f"{local}.{n}={n};") for n in names)
    problems = []
    if len(re.findall(fill, text)) != 1:
        problems.append(f"expected one run of {local}.m=m; for {names}")
    for m, v in re.findall(re.escape(local) + r"\.(\w+)=([^;]*);", text):
        if m != v.strip():
            problems.append(f"{local}.{m}={v};")
    return problems


def check_position(text, helper):
    if RETURNS:
        calls = list(re.finditer(r"^\s*if \(" + re.escape(helper) +
                                 r"\([^;]*?\) == MagickFalse\)\s*\n\s*return\((\w+)\);"
                                 r"\s*\n\s*(\S[^\n]*)", text, re.M))
        if len(calls) != 1:
            return [f"expected one `if ({helper}(...) == MagickFalse) return(c);`, "
                    f"found {len(calls)}"]
        if calls[0].group(2).strip() not in ("break;", "}"):
            return [f"the call is followed by '{calls[0].group(2).strip()}', not break; "
                    f"or the loop's closing brace"]
        return []
    calls = list(re.finditer(r"^\s*" + re.escape(helper) + r"\([^;]*\);\s*\n\s*(\S[^\n]*)",
                             text, re.M))
    if len(calls) != 1:
        return [f"expected one call statement of {helper}, found {len(calls)}"]
    if calls[0].group(1).strip() != "break;":
        return [f"the call is followed by '{calls[0].group(1).strip()}', not break;"]
    return []


def split_body(body, targets_ptr, input_ptr, target_names, input_names):
    """(prologue bindings, moved lines): the binding declarations that open the body,
    with their type lines, and the rest."""
    lines = [b.strip() for b in body.splitlines()]
    lines = [b for b in lines if b not in TRIVIAL]
    bound_targets, bound_inputs, problems = [], [], []
    pending, k = [], 0
    while k < len(lines):
        b = lines[k]
        m = BINDING.match(b)
        if m and m.group(2) in (targets_ptr, input_ptr):
            name, ptr, member = m.groups()
            pool = target_names if ptr == targets_ptr else input_names
            if name != member or member not in pool:
                problems.append(f"binding {b}")
            (bound_targets if ptr == targets_ptr else bound_inputs).append(member)
            pending = []
            k += 1
            continue
        if "=" not in b and ";" not in b and "(" not in b and "," not in b:
            pending.append(b)  # a type line, kept only if a binding follows
            k += 1
            continue
        break
    moved = pending + lines[k:]
    return bound_targets, bound_inputs, moved, problems


def forward(line, names):
    """A removed line as Recipe CT step 5 writes it in the helper."""
    if RETURNS:
        if line in ("break;", "continue;"):
            return "return(MagickTrue);"
        if re.fullmatch(r"return\(\w+\);", line):
            return "return(MagickFalse);"
    elif line == "break;":
        return "return;"
    for n in names:
        line = re.sub(r"(?<![\w.>])&" + n + r"\b", "\0" + n, line)
        line = re.sub(r"(?<![\w.>\0])" + n + r"\b", "*" + n, line)
        line = line.replace("\0" + n, n)
    return line


def canonical(line, names):
    for n in names:
        line = re.sub(r"\(\*" + n + r"\)", "*" + n, line)
    return line.rstrip(",;")


def caller_body(text, helper, whole):
    """The body of the function that calls HELPER (the text outside HELPER's definition,
    from the `{` that opens the enclosing function to its closing `}`)."""
    outside = text.replace(whole, "")
    call = re.search(r"^\s+(?:if \()?" + re.escape(helper) + r"\(", outside, re.M)
    if not call:
        return ""
    start = outside.rfind("\n{\n", 0, call.start())
    return outside[start:outside.find("\n}\n", call.start())]


def declaration_pairs(moved):
    """(name, type line, line) of each declaration `type` / `name;` among HELPER's lines;
    the declarator may be a pointer (`*font_info;`) or an array (`*ColorMenu[N];`)."""
    pairs = []
    for k in range(1, len(moved)):
        m = re.fullmatch(r"\**(\w+)(?:\[[^\]]*\])?;", moved[k])
        if m and not re.search(r"[=;(,]", moved[k - 1]):
            pairs.append((m.group(1), moved[k - 1], moved[k].rstrip(",;")))
    return pairs


def declares(body, name, type_line):
    """Whether BODY declares NAME under TYPE_LINE (alone or in a list of names)."""
    pattern = (r"^\s*" + re.escape(type_line) + r"\s*\n(?:\s*[^;\n]*,\s*\n)*\s*\**" + name +
               r"\s*[,;\[]")
    return re.search(pattern, body, re.M) is not None


def uses(lines, name):
    return any(re.search(r"(?<![\w.>])" + name + r"\b", line) for line in lines)


def kept_temporaries(caller, moved, removed, extra):
    """Recipe CT step 2's temporaries whose caller declaration stays: a declaration in
    the helper (a type line, then `name;`) that the removed lines do not account for,
    of a name the removed lines use, which the caller still declares with the same
    type line (kept because the caller uses the name elsewhere). Returns
    [(name, type line, declarator line)]. Whether the block writes the name before it
    reads it, and the caller does not read the value afterwards, is checked by reading
    (Recipe E step 4)."""
    found = []
    for name, type_line, line in declaration_pairs(moved):
        if min(extra[line], extra[type_line]) <= 0:
            continue
        if uses(removed, name) and declares(caller, name, type_line):
            found.append((name, type_line, line))
    return found


def unexplained(extra, temporaries, helper):
    """The problems left by HELPER's lines the removed lines do not account for."""
    for name, type_line, line in temporaries:
        extra[line] -= 1
        extra[type_line] -= 1
    return [f"in {helper} but not removed ({n}x): {line}"
            for line, n in extra.items() if n > 0]


def temporaries_note(temporaries, helper):
    return "".join(f"; temporary {name} ({type_line}) declared in {helper}, its "
                   f"declaration kept in the caller" for name, type_line, _ in temporaries)


def comment_lines(text):
    """The stripped text of the lines of FILE that lie inside a `/* ... */` comment and
    carry no code (a comment line moves with its block and is not rewritten)."""
    lines, inside = set(), False
    for raw in text.splitlines():
        line = raw.strip()
        if inside:
            if line.endswith("*/"):
                inside = False
            elif line not in TRIVIAL:
                lines.add(line)
        elif line == "/*":
            inside = True
    return lines


def diff_lines(path):
    diff = subprocess.run(["git", "diff", "-U0", "HEAD", "--", path],
                          capture_output=True, text=True, check=True).stdout
    removed, added = [], []
    for line in diff.splitlines():
        text = line[1:].strip()
        if line.startswith(("---", "+++")) or text in TRIVIAL:
            continue
        if line[:1] in "-+":
            (removed if line[0] == "-" else added).append(text)
    return removed, added


def main(argv):
    global RETURNS
    RETURNS = "--returns" in argv
    SHARED = "--shared" in argv
    argv = [a for a in argv if a not in ("--returns", "--shared")]
    if len(argv) not in (4, 6):
        raise SystemExit(__doc__)
    path, targets_type, helper = argv[1:4]
    input_type, local = (argv[4], argv[5]) if len(argv) == 6 else (None, None)
    text = open(path).read()
    target_names, targets_typedef = members(text, targets_type)
    input_names, input_typedef = members(text, input_type) if input_type else ([], "")
    whole, params, body = helper_parts(text, helper)
    problems = []
    tptr = param_name(params, targets_type)
    if not tptr or not re.search(r"const\s+" + re.escape(targets_type) +
                                 r"\s*\*\s*\w+\s*$", params.strip()):
        problems.append(f"the last parameter of {helper} is not const {targets_type} *")
    iptr = param_name(params, input_type) if input_type else None
    if input_type and not iptr:
        problems.append(f"{helper} takes no const {input_type} *")
    tlocal, bad = check_targets(text, targets_type, target_names)
    problems += bad
    if input_type:
        problems += (check_fill_shared(text, local, input_names) if SHARED
                     else check_fill(text, input_type, local, helper, input_names))
    problems += check_position(text, helper)
    bound_t, bound_i, moved, bad = split_body(body, tptr, iptr, target_names, input_names)
    problems += bad
    if not bound_t:
        problems.append("no target is bound: the block writes no variable of the caller")
    # the rewrite, as multisets
    removed, added = diff_lines(path)
    whole_lines = Counter(l.strip() for l in whole.splitlines()
                          if l.strip() not in TRIVIAL)
    outside = Counter(added)
    outside.subtract(whole_lines)
    # the typedefs' own lines (an `int` member, say) are added text of the step, not an
    # alignment of the diff: they must not cancel a removed line of the block
    outside.subtract(Counter(l.strip() for l in
                             (targets_typedef + "\n" + input_typedef).splitlines()
                             if l.strip() not in TRIVIAL))
    if RETURNS:
        # the `return(c);` the call adds to the caller is not part of the block
        call = re.search(r"^\s*if \(" + re.escape(helper) +
                         r"\([^;]*?\) == MagickFalse\)\s*\n\s*(return\(\w+\);)", text, re.M)
        if call and outside[call.group(1)] > 0:
            outside[call.group(1)] -= 1
        elif call:
            # the diff aligned the caller's `return(c);` with one of the block's: it is
            # context, so the block's copy is missing from the removed lines
            removed.append(call.group(1))
    removed_c = Counter(removed)
    for line in list(removed_c):
        same = min(removed_c[line], max(outside[line], 0))
        removed_c[line] -= same
        outside[line] -= same
    comments = comment_lines(subprocess.run(["git", "show", "HEAD:" + path],
                                            capture_output=True, text=True,
                                            errors="replace", check=True).stdout)
    want = Counter()
    for line, n in removed_c.items():
        if n > 0:
            # a line of a comment moves as it is: the rewrite does not apply to text
            moved_line = line if line in comments else forward(line, bound_t)
            want[canonical(moved_line, bound_t)] += n
    have = Counter(canonical(b, bound_t) for b in moved)
    for line, n in (want - have).items():
        problems.append(f"removed but not moved ({n}x): {line}")
    extra = have - want
    if RETURNS:
        # the helper's own last statement
        if extra["return(MagickTrue)"] < 1:
            problems.append(f"{helper} does not end with return(MagickTrue);")
        extra["return(MagickTrue)"] -= 1
    temporaries = kept_temporaries(caller_body(text, helper, whole), moved, removed, extra)
    problems += unexplained(extra, temporaries, helper)
    # what else was added outside the helper
    allowed = {l.strip() for l in (targets_typedef + "\n" + input_typedef).splitlines()}
    allowed |= {s for s in (input_type, local and local + ";") if s}
    for line, n in outside.items():
        if n <= 0 or line in allowed:
            continue
        if local and re.fullmatch(re.escape(local) + r"\.\w+=\w+;", line):
            continue  # the fill, checked above
        if re.match(r"(?:if \()?" + re.escape(helper) + r"\(", line) or line.endswith(");"):
            if helper in line or re.fullmatch(r"[\w&,\s]*\);", line):
                continue  # the call, possibly wrapped
        if tlocal and (line.startswith(f"const {targets_type}") or line.startswith(tlocal) or
                       re.fullmatch(r"(&\w+,\s*)*&\w+\s*(\};|,)?", line)):
            continue  # the targets declaration, checked above
        problems.append(f"added outside {helper} ({n}x): {line}")
    for line in problems:
        print("FAIL:", line)
    if problems:
        return 1
    print(f"OK {targets_type}: binds {', '.join(bound_t)}"
          + (f"; {input_type}: {len(input_names)} members, fill and bindings by name"
             if input_type else "")
          + temporaries_note(temporaries, helper)
          + "; rewrite and position checked")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
