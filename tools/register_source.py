#!/usr/bin/env python3
"""Register a new MagickCore source file next to an existing one (Recipe S).

    python3 tools/register_source.py pixel pixel-export

Adds MagickCore/<new>.c wherever MagickCore/<orig>.c is listed: Makefile.am
(MAGICKCORE_BASE_SRCS), Makefile.in (every entry automake generates for the
original, mirrored by hand, since automake is not installed here: the sources
list, the object and dependency lists, the dirstamp rule, the include marker,
the compile rule and the distclean lines), MagickCore/Make.com (VMS) and
scripts/format_c_api_docs. Each insertion goes right after the original's
entry. Prints how many lines it added to each file.
"""
import re
import sys

orig, new = sys.argv[1], sys.argv[2]


def edit(path, fn):
    s = open(path).read()
    t, n = fn(s)
    open(path, "w").write(t)
    print("%-28s +%d" % (path, n))


def makefile_am(s):
    a = "  MagickCore/%s.c \\\n" % orig
    assert s.count(a) == 1, "Makefile.am: %d entries" % s.count(a)
    return s.replace(a, a + "  MagickCore/%s.c \\\n" % new), 1


def makefile_in(s):
    lines = s.split("\n")
    out, n, i = [], 0, 0
    lo = "_la-%s.lo" % orig
    while i < len(lines):
        line = lines[i]
        sub = lambda x: x.replace("_la-%s." % orig, "_la-%s." % new).replace(
            "MagickCore/%s.c" % orig, "MagickCore/%s.c" % new)
        if line.startswith("MagickCore/libMagickCore_") and line.endswith(lo + ": MagickCore/%s.c" % orig):
            j = i
            while j < len(lines) and lines[j] != "":
                j += 1
            block = lines[i:j]
            out += block + [""] + [sub(x) for x in block]
            n += len(block) + 1
            i = j
            continue
        if line.startswith("MagickCore/libMagickCore_") and line.endswith(lo + ":  \\"):
            block = lines[i:i + 3]
            out += block + [sub(x) for x in block]
            n += 3
            i += 3
            continue
        if re.search(r"\tMagickCore/[\w.-]+ .*MagickCore/%s\.c( |$)" % re.escape(orig), line) or \
                re.search(r"MagickCore/%s\.c MagickCore/" % re.escape(orig), line):
            out.append(line.replace("MagickCore/%s.c" % orig,
                                    "MagickCore/%s.c MagickCore/%s.c" % (orig, new)))
            i += 1
            continue
        if ("_la-%s." % orig) in line or line == "  MagickCore/%s.c \\" % orig:
            out += [line, sub(line)]
            n += 1
            i += 1
            continue
        out.append(line)
        i += 1
    return "\n".join(out), n


def make_com(s):
    a = "$call Make %s.c\n" % orig
    assert s.count(a) == 1
    return s.replace(a, a + "$call Make %s.c\n" % new), 1


def api_docs(s):
    a = "\t '%s.c',\n" % orig
    if s.count(a) != 1:
        return s, 0
    return s.replace(a, a + "\t '%s.c',\n" % new), 1


edit("MagickCore/Makefile.am", makefile_am)
edit("Makefile.in", makefile_in)
edit("MagickCore/Make.com", make_com)
edit("scripts/format_c_api_docs", api_docs)
