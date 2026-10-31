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
"""
import collections, glob, os, re, sys

def harvest_glyphs():
    """The 5-row banner letters, as the MagickCore file headers draw them."""
    g = collections.defaultdict(collections.Counter)
    for f in glob.glob("MagickCore/*.c"):
        H = open(f, errors="replace").read().split("\n")[:20]
        rows = [l[1:-1] for l in H if l.startswith("%") and l.endswith("%") and len(l) == 79 and l.strip("% ")]
        art = [r for r in rows if re.fullmatch(r"[A-Z ]+", r)]
        if len(art) != 5: continue
        word = re.sub(r"[^A-Z]", "", f.split("/")[-1][:-2].upper())
        w = max(len(r) for r in art); art = [r.ljust(w) for r in art]
        blank = [all(r[c] == " " for r in art) for c in range(w)]
        segs = []; c = 0
        while c < w:
            if blank[c]: c += 1; continue
            d = c
            while d < w and not blank[d]: d += 1
            segs.append((c, d)); c = d
        if len(segs) != len(word): continue
        for ch, (a, b) in zip(word, segs):
            g[ch][tuple(r[a:b] for r in art)] += 1
    return {ch: list(c.most_common(1)[0][0]) for ch, c in g.items()}

args = sys.argv[1:]; out = None
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
            k = j + 1
            while k < len(L) and L[k] != "}": k += 1
            defs.append((m.group(2), i, k, bool(m.group(1) and m.group(1).startswith("static"))))
            i = k + 1; continue
    i += 1
names = {d[0] for d in defs}
body = {d[0]: "\n".join(L[d[1]:d[2] + 1]) for d in defs}
calls = {n: {x for x in names if x != n and re.search(r"\b%s\b" % re.escape(x), body[n])} for n in names}
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
cuts = []; typedefs = []
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
        break
    b = last[2] + 1
    incomment = False; intype = False; bad = []; depth = 0; inner = []
    for n in range(a, b):
        if any(s <= n <= e for s, e in spans): continue
        t = L[n].strip()
        if intype:
            intype = not re.match(r"^} \w+;$", L[n]); continue
        if re.match(r"^typedef struct _\w+$", L[n]):
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
        if re.match(r"#\s*endif\b", t) and depth > 0: depth -= 1; continue
        if t: bad.append((n + 1, t))
    if depth: bad.append(("unbalanced conditional", depth))
    for n in inner:
        e = n
        while not re.match(r"^} \w+;$", L[e]): e += 1
        typedefs.append(L[e][2:-1])
    if bad: sys.exit("refused: top-level code in the region: %s" % bad[:5])
    cuts.append((a, b))
first = order[idx[0]]
region = "\n\n".join("\n".join(L[a:b]) for a, b in cuts)
rest_lines = list(L)
for a, b in reversed(cuts):
    del rest_lines[a:b]
    # drop one of the two blank lines left at the seam
    while 1 < a < len(rest_lines) and rest_lines[a - 1] == "" and rest_lines[a] == "" and rest_lines[a - 2] == "":
        del rest_lines[a]
rest = "\n".join(rest_lines)
for tname in typedefs:
    if re.search(r"\b%s\b" % tname, rest): sys.exit("refused: type %s moves with the region but the kept part uses it" % tname)
defined = lambda text: set(re.findall(r"^\s*#\s*define\s+(\w+)", text, re.M))
for mname in defined(rest):
    if re.search(r"\b%s\b" % mname, region): sys.exit("refused: macro %s used in the region" % mname)
for mname in defined(region):
    if re.search(r"\b%s\b" % mname, rest): sys.exit("refused: macro %s used outside" % mname)
# a type the original defines and keeps must not be used by the region
types = set(re.findall(r"^typedef\s+(?:struct|enum|union)\s+(\w+)", rest, re.M)) | \
    set(re.findall(r"^\}\s*(\w+)\s*;", rest, re.M)) | \
    {t.lstrip("_") for t in re.findall(r"^struct\s+(\w+)\s*$", rest, re.M)}
for tname in sorted(types):
    if re.search(r"\b%s\b" % tname, region): sys.exit("refused: type %s defined outside the region and used in it" % tname)
# a file-scope variable the original keeps must not be used by the region
for k, line in enumerate(rest_lines):
    if not re.match(r"^static\b", line) or "(" in line: continue
    e = k
    while e < len(rest_lines) and ";" not in rest_lines[e] and "=" not in rest_lines[e]: e += 1
    decl = " ".join(rest_lines[k:e + 1]).split("=")[0]
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
assert len(art_rows) == 5, art_rows
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
inc_a = L.index("  Include declarations.") - 1
inc_b = max(k for k in range(len(L)) if L[k].startswith("#include") and k < first[1])
# carry a trailing #endif etc. that closes a conditional include
while L[inc_b + 1].startswith("#"): inc_b += 1
if defined("\n".join(L[inc_a:inc_b + 1])):
    sys.exit("refused: the include block defines macros %s; both files would define them" % sorted(defined("\n".join(L[inc_a:inc_b + 1]))))
text = "\n".join(hdr) + "\n\n" + "\n".join(L[inc_a:inc_b + 1]) + "\n\n" + region + "\n"
dst_new = os.path.join(out, os.path.basename(new)) if out else new
dst_old = os.path.join(out, os.path.basename(path)) if out else path
open(dst_new, "w").write(text)
open(dst_old, "w").write("\n".join(rest_lines))
print("typedefs %s; moved %d functions in %d runs, %d lines %s to %s" % (typedefs, len(moved), len(cuts), sum(b - a for a, b in cuts), [(a + 1, b) for a, b in cuts], dst_new))
