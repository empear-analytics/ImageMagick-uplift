#!/usr/bin/env python3
"""Recipe S: move public ROOTS, the statics only they reach and the types only they use
from FILE to NEW, verbatim and in their original order.

    python3 tools/split_source.py MagickCore/resize.c MagickCore/resize-image.c \
        "MagickCore Filtered Image Resize Method" ResizeImage [--out DIR]

With --out, both results go to DIR and the sources are left alone (a dry run to
score). Refuses when a static the family reaches has a caller outside it (its
linkage would have to widen), when anything but comments, family-only typedefs
and balanced conditionals lies between the moved functions, or when a file-level
macro crosses the boundary. NEW gets FILE's licence header, with a banner for
NEW's name (letters taken from the other MagickCore headers) and TITLE, and
FILE's include block. Register NEW with tools/register_source.py.

Also moved: a forward declaration of a moved static, and a typedef struct elsewhere
in FILE that only the family uses. A function-pointer typedef (`typedef T` / `  (*Name)(...);`)
between or directly above the moved functions moves with them when the kept part names none
of its names (it refuses the split when the kept part does). A function that names a macro
calls what the macro's definitions name, in every branch and through further macros
(`#define ScaledSig(x) ... ScaledSigmoidal(...)`), so a static reached only through a macro
moves with its callers, and one the kept part also calls is shared. --copy-macro NAME (repeatable) defines a one-line
`#define NAME "literal"` of the kept part again in NEW (Recipe S step 2). A file-scope
`static const` table between the moved functions moves with them, and a one-line
`#define` of the original that only the family uses moves to NEW (step 2 as amended
2026-11-11), each refused when the kept part names it; a forward declaration
of a moved static at the top of FILE moves to NEW: a `static T` line and its declarators, each possibly wrapped
over several lines; a group that also declares a kept static is split, the moved declarators going to NEW under
a copy of the `static T` line, and a group inside an `extern "C"` block is declared inside one in NEW too. A trailing run of conditional lines
in FILE's include block that holds every macro the block defines (a delegate's headers)
is left out of NEW's copy when the family names none of those macros. A quoted `#include` of a header of the same directory that defines objects (a file-scope initialised
variable, as accelerate-kernels-private.h's `accelerateKernels`) is left out of NEW's copy of the include
block, since both files would define the variable, when no line of the family names a variable or macro
that header defines; the split is refused when one does. A file-level
`#if defined(...)` ... `#else` ... `#endif` block that holds whole functions of the family
(a delegate's code and its stub of the same public function) moves whole, its directives
included, when its `#endif` follows the family's last function; a function defined in both
branches calls what either definition calls (owner, 2026-11-11).

When the family stands inside a file-level conditional that the rest of FILE keeps (a delegate's
`#if defined(MAGICKCORE_X11_DELEGATE)` that wraps the whole file), NEW carries that conditional too:
its `#if` line, if the copied include block does not hold it, the family, and its `#endif`, every
directive verbatim. Family functions standing in the `#else` branch of that conditional (the stub of a
moved public function) are carried under a copy of the `#else` line. A family that stands in an `#elif`
branch, in different conditionals, or in the `#else` branch of an outer conditional is refused, and so
is a NEW whose directives do not balance.
"""
import collections, glob, os, re, sys

def banner_art(f):
    """The five letter rows of the banner at the head of F, padded to one width, or []."""
    head = open(f, errors="replace").read().split("\n")[:20]
    rows = [l[1:-1] for l in head if re.fullmatch(r"%.{77}%", l) and l.strip("% ")]
    art = [r for r in rows if re.fullmatch(r"[A-Z ]+", r)]
    width = max([len(r) for r in art] + [0])
    return [r.ljust(width) for r in art] if len(art) == 5 else []

def letter_spans(art):
    """The (start, end) columns of each letter that the rows ART draw, blank columns between."""
    mask = "".join("x" if "".join(column).strip() else " " for column in zip(*art))
    return [m.span() for m in re.finditer(r"x+", mask)]

def harvest_glyphs():
    """The 5-row banner letters, as the MagickCore file headers draw them."""
    g = collections.defaultdict(collections.Counter)
    for f in glob.glob("MagickCore/*.c"):
        art = banner_art(f)
        word = re.sub(r"[^A-Z]", "", f.split("/")[-1][:-2].upper())
        segs = letter_spans(art)
        if art and len(segs) == len(word):
            for ch, (a, b) in zip(word, segs): g[ch][tuple(r[a:b] for r in art)] += 1
    return {ch: list(c.most_common(1)[0][0]) for ch, c in g.items()}

def conditional_start(L, end):
    """The line of the #if that the #endif at L[end] closes."""
    depth = 0
    for u in range(end, -1, -1):
        w = L[u].strip()
        if w.startswith("#endif"): depth += 1
        elif re.match(r"#\s*(if|ifdef|ifndef)\b", w): depth -= 1
        if depth == 0: return u
    return None

def conditional_records(L):
    """Every #if ... #endif of L as {"if": line, "else": [lines of its #else/#elif], "end": line}, by #if line."""
    recs = []; stack = []
    for n, line in enumerate(L):
        w = line.strip()
        if re.match(r"#\s*(if|ifdef|ifndef)\b", w): recs.append({"if": n, "else": [], "end": None}); stack.append(recs[-1])
        elif re.match(r"#\s*(else|elif)\b", w) and stack: stack[-1]["else"].append(n)
        elif re.match(r"#\s*endif\b", w) and stack: stack.pop()["end"] = n
    return recs

def enclosing_conditionals(recs, a):
    """The conditionals of RECS that are open at line A, outermost first, with the branch (0 for #if) A stands in."""
    return [(r, sum(1 for e in r["else"] if e < a)) for r in recs if r["if"] < a and (r["end"] is None or r["end"] > a)]

def open_directives(lines):
    """The #if lines still open at the end of LINES, outermost first; None when an #endif or #else has no #if."""
    stack = []
    for line in lines:
        w = line.strip()
        if re.match(r"#\s*(if|ifdef|ifndef)\b", w): stack.append(w)
        elif re.match(r"#\s*(else|elif)\b", w) and not stack: return None
        elif re.match(r"#\s*endif\b", w):
            if not stack: return None
            stack.pop()
    return stack

def object_header_names(path):
    """The names a header at PATH defines that make it unfit for two files: each file-scope initialised object
    (`const char *name =` in column 0) and each #define; empty when PATH has no such object definition."""
    if not os.path.exists(path): return set()
    text = re.sub(r"/\*.*?\*/", "", open(path, errors="replace").read(), flags=re.S)
    objects = set(re.findall(r"(?m)^(?!typedef|static|extern|return|#)(?:const\s+)?\w[\w \t\*]*?[ \t\*](\w+)\s*(?:\[[^\]]*\])?\s*=(?!=)", text))
    return objects | set(re.findall(r"(?m)^\s*#\s*define\s+(\w+)", text)) if objects else set()

def macro_block_start(L, c):
    """First line of the #define, or the #if ... #endif holding one, that ends at L[c - 1]."""
    if L[c - 1].strip().startswith("#endif"):
        u = conditional_start(L, c - 1)
        block = "\n".join(L[u:c]) if u is not None else ""
        return u if "#define" in block and "#include" not in block else None
    u = c - 1
    while u > 0 and L[u - 1].rstrip().endswith("\\"): u -= 1
    return u if L[u].lstrip().startswith("#define") else None

def typedef_names(L, starts):
    """The names closing each `typedef struct [_X]` that begins at one of the lines STARTS."""
    names = []
    for n in starts:
        while not re.match(r"^} \w+;$", L[n]): n += 1
        names.append(L[n][2:-1])
    return names

def continues_above(L, s):
    """True when the statement that L[s] belongs to goes on above it: L[s] is no `typedef` line, and the line above is
    neither blank nor the end of a statement, a block or a comment."""
    if L[s].startswith("typedef"): return False
    above = L[s - 1].rstrip()
    return bool(above) and not above.endswith((";", "{", "}", "*/"))

def function_pointer_typedef(L, e):
    """(first line, names) of a `typedef T` / `  (*Name)(...);` function-pointer typedef (one or more declarators)
    that ends at L[e], else None."""
    s = e
    while s > 0 and continues_above(L, s): s -= 1
    text = " ".join(x.strip() for x in L[s:e + 1])
    if not re.fullmatch(r"typedef[^;{}]*\(\*\w+\)\([^;{}]*\);", text): return None
    return s, re.findall(r"\(\*(\w+)\)", text)

def static_table(L, n):
    """(last line, name) of a file-scope `static const` table declared from L[n], else None."""
    if not re.match(r"^static const\b[^(]*$", L[n]): return None
    e = next(k for k in range(n, len(L)) if re.match(r"^[^{]*;\s*$|^\s*\};\s*$", L[k]))
    decl = " ".join(x.strip() for x in L[n:e + 1]).split("=")[0].strip()
    return e, re.findall(r"(\w+)\s*(?:\[[^\]]*\])?$", decl)[0]

def family_only_define(lines, name):
    """The one-line `#define NAME ...` of LINES when no other line of LINES names NAME, else None."""
    named = [l for l in lines if re.search(r"\b%s\b" % name, l)]
    one_line = [l for l in named if re.match(r"^#define\s+%s\b.*[^\\\s]\s*$" % name, l)]
    return named[0] if named == one_line[:1] else None

def drop_empty_banner(lines, banner):
    """Remove a `/* banner */` comment block of LINES that has nothing but blank lines under it."""
    text = re.sub(r"(?m)^/\*\n  %s\n\*/\n[\n\f]*(?=/\*)" % re.escape(banner), "", "\n".join(lines), count=1)
    lines[:] = text.split("\n")

EXTERN_C_OPEN = ["#if defined(__cplusplus) || defined(c_plusplus)", 'extern "C" {', "#endif"]
EXTERN_C_CLOSE = ["#if defined(__cplusplus) || defined(c_plusplus)", "}", "#endif"]

def in_extern_c(lines, k):
    """Whether line K of LINES stands inside an `extern "C" {` block written as EXTERN_C_OPEN ... EXTERN_C_CLOSE."""
    depth = 0
    for i in range(k):
        if lines[i:i + 3] == EXTERN_C_OPEN: depth += 1
        elif lines[i:i + 3] == EXTERN_C_CLOSE: depth -= 1
    return depth > 0

def declarator_end(lines, e):
    """The index of the line where the declarator starting at line E closes its parentheses, or None when that
    line does not end the declarator with `,` or `;`."""
    depth = 0
    for k in range(e, len(lines)):
        depth += lines[k].count("(") - lines[k].count(")")
        if depth == 0:
            return k if re.search(r"\)[,;]$", lines[k].rstrip()) else None
    return None

def declaration_group(lines, k):
    """The declarators of a forward declaration group `static T` / `  *?Name(...),` ... `  *?Name(...);` starting at
    line K, as (name, lines) pairs, and the index of the group's last line; None when K starts no such group. A
    declarator may run over several lines (its parameter list wrapped): it ends on the line where its parentheses
    close, which must end in `,` or `;`."""
    if not re.fullmatch(r"static [\w \*]+", lines[k]): return None
    decls = []; e = k + 1
    while e < len(lines):
        m = re.match(r"^  \*?(\w+)\(", lines[e])
        end = declarator_end(lines, e) if m else None
        if end is None: return None
        decls.append((m.group(1), lines[e:end + 1]))
        if lines[end].rstrip().endswith(";"): return decls, end
        e = end + 1
    return None

def group_text(head, decls):
    """The group HEAD / DECLS as text, its last declarator ending in `;` and the others in `,`."""
    out = [head]
    for n, (_, dl) in enumerate(decls):
        dl = list(dl)
        dl[-1] = dl[-1].rstrip()[:-1] + (";" if n == len(decls) - 1 else ",")
        out += dl
    return "\n".join(out)

def is_blank_around(lines, k, e):
    """Whether blank lines stand directly before line K and directly after line E."""
    return 0 < k and e + 1 < len(lines) and lines[k - 1] == "" and lines[e + 1] == ""

def split_group(lines, k, statics, taken):
    """Take the declarators of STATICS out of the group at line K of LINES into TAKEN; return the next line."""
    g = declaration_group(lines, k)
    if g is None: return k + 1
    decls, e = g
    moved = [d for d in decls if d[0] in statics]
    if not moved: return e + 1
    kept = [d for d in decls if d[0] not in statics]
    taken.append((group_text(lines[k], moved), in_extern_c(lines, k)))
    repl = group_text(lines[k], kept).split("\n") if kept else []
    if not kept and is_blank_around(lines, k, e):
        e += 1  # the blank line that separated the group from the next one goes with it
    lines[k:e + 1] = repl
    return k + len(repl)

def take_forward_declarations(lines, statics):
    """Remove from LINES the forward declarations of STATICS; return them as (text, in_extern_c) pairs.

    A declaration is a group: a `static T` line, then one or more declarators `  Name(...)` (or `  *Name(...)`),
    each possibly wrapped over several lines, separated by `,` and ended by `;`. A group whose declarators all
    name STATICS goes whole. A group that also declares a kept static is split: the kept declarators stay under
    its `static T` line, in their order, and the moved ones form a group of their own under a copy of that line.
    A group inside an `extern "C"` block (stream.c's pixel cache interfaces) is reported so, for the new file
    to declare it inside one as well."""
    taken = []; k = 0
    while k < len(lines):
        k = split_group(lines, k, statics, taken)
    return taken

def forward_block(forward):
    """The new file's *Forward declarations* block for FORWARD, (text, in_extern_c) pairs; the declarations taken
    from an `extern "C"` block stand in one, written as the original writes it."""
    items = [t for t, c in forward if not c]
    wrapped = [t for t, c in forward if c]
    if wrapped:
        items.append("\n".join(EXTERN_C_OPEN) + "\n\n" + "\n\n".join(wrapped) + "\n\n" + "\n".join(EXTERN_C_CLOSE))
    return declaration_block("Forward declarations.", items)

def relocate_declarations(lines, statics, macros):
    """Take the forward declarations of STATICS out of LINES, and the banners the moves leave empty."""
    if macros: drop_empty_banner(lines, "Define declarations.")
    forward = take_forward_declarations(lines, statics)
    if forward: drop_empty_banner(lines, "Forward declarations.")
    return forward

def declaration_block(title, items, sep="\n"):
    """A `/* title */` block of ITEMS for the new file's head, or nothing when ITEMS is empty."""
    return "/*\n  %s\n*/\n" % title + sep.join(items) + "\n\n" if items else ""

def brace_delta(line):
    """Opening minus closing braces on LINE, outside string and character literals and comments."""
    t = re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', "", line)
    t = re.sub(r"/\*.*?\*/|//.*$", "", t)
    return t.count("{") - t.count("}")

args = sys.argv[1:]; out = None; copy_macros = []
while "--copy-macro" in args: k = args.index("--copy-macro"); copy_macros.append(args[k + 1]); del args[k:k + 2]
if "--out" in args: k = args.index("--out"); out = args[k + 1]; del args[k:k + 2]
path, new, title, roots = args[0], args[1], args[2], args[3:]
src = open(path).read(); L = src.split("\n")
defs = []; i = 0
while i < len(L):
    m = re.match(r"^(static\s+(?:inline\s+)?|MagickExport\s+|MagickPrivate\s+)?[A-Za-z_][\w \*]*?\b([A-Za-z_]\w*)\s*\(", L[i])
    if m and not L[i].rstrip().endswith(";") and not L[i].startswith((" ", "#", "%", "/*")):
        j = i
        while j < len(L) and L[j] != "{" and not L[j].rstrip().endswith(";"): j += 1
        if j < len(L) and L[j] == "{":
            k = j + 1; depth = 1; alt = []
            # the closing brace of a function stands alone in column 0; a macro body's own `}` follows a line ending in `\`,
            # and a column-0 block inside the body (`{` ... `}` under an #if) keeps the brace depth above zero; braces
            # in an #else or #elif branch are not counted (the first branch alone is a whole function)
            while k < len(L):
                w = L[k].strip()
                if re.match(r"#\s*(if|ifdef|ifndef)\b", w): alt.append(False)
                elif re.match(r"#\s*(else|elif)\b", w):
                    if alt: alt[-1] = True
                    else: alt.append(True)
                elif re.match(r"#\s*endif\b", w):
                    if alt: alt.pop()
                elif not any(alt): depth += brace_delta(L[k])
                if L[k] == "}" and not L[k - 1].rstrip().endswith("\\") and depth == 0: break
                k += 1
            defs.append((m.group(2), i, k, bool(m.group(1) and m.group(1).startswith("static"))))
            i = k + 1; continue
    i += 1
names = {d[0] for d in defs}
# a name that only a comment mentions ("Contrast limited ...") is not a call
# a function defined once in each branch of an #if/#else (a delegate's code and its stub) calls what either one calls
body = collections.defaultdict(str)
for d in defs: body[d[0]] += re.sub(r"/\*.*?\*/", "", "\n".join(L[d[1]:d[2] + 1]), flags=re.S) + "\n"
# a macro's body names functions too (`#define ScaledSig(x) ... ScaledSigmoidal(...)`): a function that names a macro
# calls what the macro's definitions name, in every branch of a conditional, through further macros
macro_text = collections.defaultdict(str)
for k, line in enumerate(L):
    dm = re.match(r"^#\s*define\s+(\w+)", line)
    if not dm: continue
    e = k
    while L[e].rstrip().endswith("\\") and e + 1 < len(L): e += 1
    macro_text[dm.group(1)] += re.sub(r"/\*.*?\*/", "", "\n".join(L[k:e + 1]), flags=re.S) + "\n"
def macro_expansion(text):
    """TEXT and the text of every macro definition it names, through further macros."""
    seen = set(); out = [text]; todo = [text]
    while todo:
        t = todo.pop()
        for mname in macro_text:
            if mname not in seen and re.search(r"\b%s\b" % re.escape(mname), t):
                seen.add(mname); out.append(macro_text[mname]); todo.append(macro_text[mname])
    return "\n".join(out)
calls = {n: {x for x in names if x != n and re.search(r"\b%s\b" % re.escape(x), macro_expansion(body[n]))} for n in names}
static = {d[0] for d in defs if d[3]}
reach = set(roots); st = list(roots)
while st:
    n = st.pop()
    for c in calls[n]:
        if c in static and c not in reach: reach.add(c); st.append(c)
excl = {r for r in reach if r in static}; ch = True
while ch:
    ch = False
    for s in list(excl):
        if not {n for n in names if s in calls[n]} <= (excl | set(roots)): excl.discard(s); ch = True
shared = sorted(r for r in reach if r in static and r not in excl)
if shared: sys.exit("refused: shared statics %s" % shared)
moved = excl | set(roots)
order = [d for d in defs]
idx = [k for k, d in enumerate(order) if d[0] in moved]
runs = []
for k in idx:
    if runs and k == runs[-1][-1] + 1: runs[-1].append(k)
    else: runs.append([k])
spans = [(d[1], d[2]) for d in order if d[0] in moved]
cuts = []; typedefs = []; tables = []
recs = conditional_records(L)
for run in runs:
    first, last = order[run[0]], order[run[-1]]
    # start: the comment directly above the first function
    # start: the comments and family-only typedefs directly above the first function
    a = first[1]
    while True:
        c = a
        while c > 0 and L[c - 1].strip() == "": c -= 1
        if L[c - 1].strip() == "*/":
            c -= 1
            while not L[c].startswith("/*"): c -= 1
            a = c; continue
        m = re.match(r"^} (\w+);$", L[c - 1])
        if m:
            t = c - 1
            while not L[t].startswith("typedef"): t -= 1
            outside = "\n".join(L[:t] + L[last[2] + 1:])
            if not re.search(r"\b%s\b" % m.group(1), outside):
                a = t; typedefs.append(m.group(1)); continue
        fp = function_pointer_typedef(L, c - 1)
        if fp and not any(re.search(r"\b%s\b" % n, "\n".join(L[:fp[0]] + L[last[2] + 1:])) for n in fp[1]):
            a = fp[0]; continue
        u = macro_block_start(L, c)
        if u is None: break
        a = u
    b = last[2] + 1
    incomment = False; intype = False; bad = []; depth = 0; inner = []; inproto = False; table_end = -1; fp_end = -1
    for n in range(a, b):
        if any(s <= n <= e for s, e in spans): continue
        t = L[n].strip()
        if inproto:
            inproto = not t.endswith(";"); continue
        mp = re.match(r"^static\s+[\w \*]*?\b([A-Za-z_]\w*)\s*\(", L[n])
        if mp and mp.group(1) in moved and mp.group(1) in static:
            inproto = not t.endswith(";"); continue
        if intype:
            intype = not re.match(r"^} \w+;$", L[n]); continue
        if n <= fp_end: continue
        if L[n].startswith("typedef"):
            e = next(k for k in range(n, len(L)) if L[k].rstrip().endswith(";"))
            fp = function_pointer_typedef(L, e)
            if fp and fp[0] == n:
                fp_end = e; typedefs.extend(fp[1]); continue
        if n <= table_end: continue
        table = static_table(L, n)
        if table:
            table_end = table[0]; tables.append(table[1]); continue
        if re.match(r"^typedef struct( _\w+)?$", L[n]):
            intype = True; inner.append(n); continue
        if t.startswith("#define") or (n > 0 and L[n - 1].rstrip().endswith("\\")):
            continue
        if incomment:
            if "*/" in t: incomment = False
            continue
        if t.startswith("/*"):
            incomment = "*/" not in t[2:]; continue
        if re.match(r"#\s*(if|ifdef|ifndef)\b", t): depth += 1; continue
        if re.match(r"#\s*(else|elif)\b", t) and depth > 0: continue
        # the #else of the conditional the whole run stands in (a moved function and its stub, side by side)
        if re.match(r"#\s*else\b", t) and any(r["if"] < a and n in r["else"] and r["end"] >= b for r in recs): continue
        if re.match(r"#\s*endif\b", t) and depth > 0: depth -= 1; continue
        if t: bad.append((n + 1, t))
    # a file-level #if opened in the region whose #endif follows its last function moves whole, #endif included
    while depth and b < len(L) and re.match(r"#\s*endif\b", L[b].strip()): depth -= 1; b += 1
    if depth: bad.append(("unbalanced conditional", depth))
    typedefs.extend(typedef_names(L, inner))
    if bad: sys.exit("refused: top-level code in the region: %s" % bad[:5])
    cuts.append((a, b))
first = order[idx[0]]
region = "\n\n".join("\n".join(L[a:b]) for a, b in cuts)
rest_lines = list(L)
# the #else of the conditional that a whole run stands in (a moved function and its stub, side by side) stays in FILE:
# the run is cut in two around it, and NEW takes the whole run, #else included
stays = {n for a, b in cuts for r in recs if r["if"] < a and r["end"] >= b for n in r["else"] if a <= n < b}
deletions = []
for a, b in cuts:
    start = a
    for n in sorted(x for x in stays if a <= x < b):
        deletions.append((start, n)); start = n + 1
    deletions.append((start, b))
for a, b in reversed(deletions):
    del rest_lines[a:b]
    # drop one of the two blank lines left at the seam
    while 1 < a < len(rest_lines) and rest_lines[a - 1] == "" and rest_lines[a] == "" and rest_lines[a - 2] == "":
        del rest_lines[a]
rest = "\n".join(rest_lines)
for tname in typedefs + tables:
    if re.search(r"\b%s\b" % tname, rest): sys.exit("refused: %s moves with the region but the kept part uses it" % tname)
defined = lambda text: set(re.findall(r"^\s*#\s*define\s+(\w+)", text, re.M))
macro_lines = []; moved_macros = []
for mname in defined(rest):
    if not re.search(r"\b%s\b" % mname, region): continue
    dl = [l for l in rest_lines if re.match(r"^#define\s+%s\b" % mname, l)]
    if mname in copy_macros and len(dl) == 1 and re.fullmatch(r'#define\s+%s\s+"[^"\\]*"' % mname, dl[0]) \
        and not re.search(r"^\s*#\s*define\s+%s\b" % mname, region, re.M):
        macro_lines.append(dl[0]); continue
    only = family_only_define(rest_lines, mname)
    if only is None: sys.exit("refused: macro %s used in the region" % mname)
    moved_macros.append(only); rest_lines.remove(only)
forward = relocate_declarations(rest_lines, moved & static, moved_macros)
rest = "\n".join(rest_lines)
for mname in defined(region):
    if re.search(r"\b%s\b" % mname, rest): sys.exit("refused: macro %s used outside" % mname)
# a typedef elsewhere in the original that only the region uses moves with it
moved_types = []
for tname in sorted(set(re.findall(r"^\}\s*(\w+)\s*;", rest, re.M))):
    if not re.search(r"\b%s\b" % tname, region): continue
    t1 = next(k for k, l in enumerate(rest_lines) if l == "} %s;" % tname)
    t0 = next(k for k in range(t1, -1, -1) if re.match(r"^typedef struct( _\w+)?$", rest_lines[k]))
    others = "\n".join(rest_lines[:t0] + rest_lines[t1 + 1:])
    if re.search(r"\b%s\b" % tname, others): continue
    moved_types.append("\n".join(rest_lines[t0:t1 + 1]))
    e = t1 + 1
    if e < len(rest_lines) and rest_lines[e] == "": e += 1
    del rest_lines[t0:e]
    # a "Typedef declarations." comment left with nothing under it goes too
    if rest_lines[t0 - 3:t0] == ["/*", "  Typedef declarations.", "*/"] and \
        not rest_lines[t0].startswith("typedef"):
        del rest_lines[t0 - 3:t0]
    rest = "\n".join(rest_lines)
# a type the original defines and keeps must not be used by the region
types = set(re.findall(r"^typedef\s+(?:struct|enum|union)\s+(\w+)", rest, re.M)) | \
    set(re.findall(r"^\}\s*(\w+)\s*;", rest, re.M)) | \
    {t.lstrip("_") for t in re.findall(r"^struct\s+(\w+)\s*$", rest, re.M)}
for k, line in enumerate(rest_lines):
    if line.startswith("typedef") and "(*" in line + " ".join(rest_lines[k + 1:k + 2]):
        e = next(x for x in range(k, len(rest_lines)) if rest_lines[x].rstrip().endswith(";"))
        fp = function_pointer_typedef(rest_lines, e)
        if fp and fp[0] == k: types.update(fp[1])
for tname in sorted(types):
    if re.search(r"\b%s\b" % tname, region): sys.exit("refused: type %s defined outside the region and used in it" % tname)
# a file-scope variable the original keeps must not be used by the region
for k, line in enumerate(rest_lines):
    if not re.match(r"^static\b", line) or "(" in line: continue
    e = k
    while e < len(rest_lines) and ";" not in rest_lines[e] and "=" not in rest_lines[e]: e += 1
    decl = " ".join(rest_lines[k:e + 1]).split("=")[0]
    if rest_lines[k + 1:k + 2] == ["{"]:
        # `static struct` / `{` members `}` declarator: the variable is named after the closing brace
        e = next(x for x in range(k, len(rest_lines)) if rest_lines[x].startswith("}"))
        decl = rest_lines[e][1:].split("=")[0]
    elif "(" in decl: continue  # a forward declaration of functions (`static double` / `  I0(double x),`)
    for vname in re.findall(r"(\w+)\s*(?:\[[^\]]*\])?\s*(?:,|;|$)", decl.strip()):
        if re.search(r"\b%s\b" % vname, region): sys.exit("refused: file-scope variable %s defined outside the region and used in it" % vname)
# a non-static function the region calls must be declared in a header
headers = "\n".join(open(h).read() for h in glob.glob(os.path.join(os.path.dirname(path), "*.h")))
for n in sorted(names - moved - static):
    if any(n in calls[m] for m in moved) and not re.search(r"\b%s\s*\(" % n, headers):
        sys.exit("refused: %s is called from the region but declared in no header" % n)
# header: the original's, with the new name's banner and the given title
hend = L.index("*/")
hdr = L[:hend + 1]
art_rows = [k for k in range(hend) if re.fullmatch(r"%[A-Z ]+%", L[k]) and L[k].strip("% ")]
assert art_rows and len(art_rows) % 5 == 0, art_rows
g = harvest_glyphs()
row = lambda t, w: "%" + (" " * ((77 - w) // 2) + t.rstrip()).ljust(77) + "%"
words = [re.sub(r"[^A-Za-z]", "", x).upper() for x in os.path.basename(new)[:-2].split("-")]
blocks = []
for word in words:
    art = ["  ".join(g[c][r] for c in word) for r in range(5)]
    w = max(len(x) for x in art)
    blocks.append([row(x, w) for x in art])
tk = [k for k in range(art_rows[-1] + 1, hend) if L[k].strip("% ")][0]
blank = row("", 0)
art_lines = blocks[0]
for blk in blocks[1:]: art_lines = art_lines + [blank] + blk
hdr = L[:art_rows[0]] + art_lines + L[art_rows[-1] + 1:tk] + [row(title, len(title))] + L[tk + 1:hend + 1]
assert all(len(x) == 79 for x in art_lines), "header width"
# the include block starts at its "Include declarations." comment, or, in a file that has none
# (vision.c), at the first #include after the licence header
inc_a = next((k - 1 for k, l in enumerate(L) if l.strip() == "Include declarations."), None)
if inc_a is None: inc_a = next(k for k in range(hend, len(L)) if L[k].startswith("#include"))
inc_b = max(k for k in range(len(L)) if L[k].startswith("#include") and k < first[1])
# carry a trailing #endif etc. that closes a conditional include
while L[inc_b + 1].startswith("#"): inc_b += 1
if defined("\n".join(L[inc_a:inc_b + 1])):
    # a trailing run of conditional lines (a delegate's headers and its macros) that only the kept part needs is
    # left out of the new file's include block (Recipe S step 2, 2026-11-11): every #define of the block stands in
    # that run, the run is preprocessor lines only and balanced, and the region names none of its macros
    last = inc_a; depth = 0
    for k in range(inc_a, inc_b + 1):
        w = L[k].strip()
        if re.match(r"#\s*(if|ifdef|ifndef)\b", w): depth += 1
        elif re.match(r"#\s*endif\b", w): depth -= 1
        elif depth == 0 and w.startswith("#include"): last = k
    tail = L[last + 1:inc_b + 1]
    tail_macros = defined("\n".join(tail))
    code = re.sub(r'"(?:\\.|[^"\\])*"', '""', re.sub(r"/\*.*?\*/", "", region, flags=re.S))
    named = sorted(m for m in tail_macros if re.search(r"\b%s\b" % m, code))
    tail_ok = all(t.strip().startswith("#") for t in tail if t.strip()) and \
        not defined("\n".join(L[inc_a:last + 1])) and not named
    if not tail_ok:
        sys.exit("refused: the include block defines macros %s; both files would define them%s" %
            (sorted(defined("\n".join(L[inc_a:inc_b + 1]))), " (the region names %s)" % named if named else ""))
    print("include block: the conditional lines %d-%d (macros %s) stay in the original only" %
        (last + 2, inc_b + 1, sorted(tail_macros)))
    inc_b = last
include_lines = L[inc_a:inc_b + 1]
region_code = re.sub(r'"(?:\\.|[^"\\])*"', '""', re.sub(r"/\*.*?\*/", "", region, flags=re.S))
for k, line in enumerate(list(include_lines)):
    mi = re.match(r'^#include "(?:[\w.-]+/)*([\w.-]+\.h)"\s*$', line)
    names_in = object_header_names(os.path.join(os.path.dirname(path), mi.group(1))) if mi else set()
    if not names_in: continue
    named = sorted(n for n in names_in if re.search(r"\b%s\b" % n, region_code))
    if named: sys.exit("refused: the family names %s, defined by %s, which defines objects and cannot be included by both files" % (named, mi.group(1)))
    include_lines[k] = None
    print("include block: %s defines objects and stays in the original only" % mi.group(1))
include_lines = [l for l in include_lines if l is not None]
extra = declaration_block("Typedef declarations.", moved_types, "\n\n") + \
    declaration_block("Define declarations.", moved_macros + macro_lines) + \
    forward_block(forward)
# the file-level conditionals the family stands in (a delegate's #if around the whole file) go to NEW as well
runs_in = [enclosing_conditionals(recs, a) for a, b in cuts]
if any([r["if"] for r, _ in e] != [r["if"] for r, _ in runs_in[0]] for e in runs_in):
    sys.exit("refused: the moved runs stand in different conditionals")
enc = [r for r, _ in runs_in[0]]
branch = [[k for _, k in e] for e in runs_in]
for lvl, r in enumerate(enc):
    inner = lvl == len(enc) - 1
    ks = {b[lvl] for b in branch}
    if not inner and ks != {0}:
        sys.exit("refused: the family stands in the #else or #elif branch of the outer conditional at line %d" % (r["if"] + 1))
    if r["end"] is None: sys.exit("refused: the conditional at line %d has no #endif" % (r["if"] + 1))
    if ks - {0} and (ks - {0, 1} or not re.match(r"#\s*else\b", L[r["else"][0]].strip())):
        sys.exit("refused: the family stands in an #elif branch of the conditional at line %d" % (r["if"] + 1))
pre = [L[r["if"]] for r in enc if r["if"] < inc_a]
post = [L[r["if"]] for r in enc if r["if"] > inc_b]
closers = [L[r["end"]] for r in reversed(enc)]
pieces = []; cur = 0
for (a, b), br in zip(cuts, branch):
    k = br[-1] if br else 0
    if k != cur: pieces.append(L[enc[-1]["else"][0]]); cur = k
    pieces.append("\n".join(L[a:b]))
region_out = "\n\n".join(pieces)
text = "\n".join(hdr) + "\n\n" + "".join(l + "\n" for l in pre) + "\n".join(include_lines) + "\n" + \
    "".join(l + "\n" for l in post) + "\n" + extra + region_out + "\n" + "".join(l + "\n" for l in closers)
if enc:
    head = text[:text.index(region_out)].split("\n")
    if open_directives(head) != [L[r["if"]].strip() for r in enc]:
        sys.exit("refused: the new file's include block does not open the conditionals the family stands in")
    print("conditionals carried: %s" % [(r["if"] + 1, r["end"] + 1) for r in enc])
if open_directives(text.split("\n")) != []:
    sys.exit("refused: the new file's #if, #else and #endif lines do not balance")
dst_new = os.path.join(out, os.path.basename(new)) if out else new
dst_old = os.path.join(out, os.path.basename(path)) if out else path
open(dst_new, "w").write(text)
open(dst_old, "w").write("\n".join(rest_lines))
print("typedefs %s; moved %d functions in %d runs, %d lines %s to %s" % (typedefs, len(moved), len(cuts), sum(b - a for a, b in cuts), [(a + 1, b) for a, b in cuts], dst_new))
