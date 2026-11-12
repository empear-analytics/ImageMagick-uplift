#!/usr/bin/env python3
"""split_branch_check.py OLDREV:OLDFILE NEWFILE BUILD

Recipe S, *Preprocessor branches* exception of 2026-11-11: a split may move an `#if defined(...)` branch
verbatim with the function that holds it when a build that compiles that branch shows the moved functions'
instructions unchanged. This compiles the original file as it stood at OLDREV and the new file as it is in
the working tree, both with the flags of build-oracle/BUILD (cand-wide for X11/OpenCL, ci-gcc-min or
ci-gcc-dlg for the non-HDRI branches, ci-gcc-noatanh for the `#else` side of MAGICKCORE_HAVE_ATANH:
tools/ci_linux_warnings.sh configures it), at -g0, disassembles every function the new file defines in both
objects, and compares them function by function, matched by name (so the order the compiler laid the
functions out in does not matter), line by line. Allowed differences, each counted in the output:
  - a `__LINE__` immediate (`mov $N,%ecx` / `%edx`) shifted by a constant per function,
  - a .rodata reference (resolved to the 16 bytes it points at; `__FILE__` strings differ),
  - a compiler constant-pool label number (.LCPI<n>_<m>), a rip-relative offset,
  - the label of a call or jump the linker resolves (`call <name>` followed by its relocation line: objdump prints the
    symbol at the next instruction, which for a call ending a function is whichever function the object lays out next);
    the relocation line that names the real target is compared as every other line,
  - a string-literal label number (clang's .L.str.<n>), when both labels hold the same bytes: each
    reference is resolved to the NUL-terminated string the label points at (the path the object was
    compiled from reads as __FILE__ in both), so a changed literal still prints DIFFERENT.
Prints IDENTICAL-MODULO-LAYOUT or DIFFERENT (with the first differing lines) and exits 0 or 1.
"""
import os, re, subprocess, sys, tempfile

def sh(*a, **k):
    return subprocess.run(a, capture_output=True, **k)

def flags(build):
    mk = open(os.path.join(build, "Makefile")).read()
    get = lambda n: re.search(r"^%s = (.*)$" % n, mk, re.M).group(1)
    return get("CC"), get("CPPFLAGS").split() + get("CFLAGS").split()

def section_bytes(obj, sec):
    return sh("objcopy", "-O", "binary", "--only-section=" + sec, obj, "/dev/stdout").stdout

def string_labels(obj, src):
    """Map each local string label (.L.str.<n>) of OBJ to the hex of the NUL-terminated bytes it points at."""
    symbols = re.findall(r"(?m)^([0-9a-f]+) .{7} (\S+)\s+[0-9a-f]+ (\.L\.str(?:\.\d+)?)$", sh("objdump", "-t", obj, text=True).stdout)
    sections = {sec: section_bytes(obj, sec) for _, sec, _ in symbols}
    texts = {label: sections[sec][int(at, 16):].split(b"\0", 1)[0] for at, sec, label in symbols}
    return {label: "__FILE__" if text == src.encode() else text.hex() for label, text in texts.items()}

def mark_call_placeholders(lines):
    """A call or jump the assembler left to the linker has an operand of 0, which objdump prints as the symbol at the
    next instruction: the function's own name in the middle of a function, and whichever function the object lays out
    next when the call is the last instruction (a call to __assert_fail that ends a function). That label is the layout,
    not the code; the relocation line that follows names the real target and is still compared line by line. The label
    is marked <placeholder:NAME> so that classify() can count a difference in NAME alone."""
    for i in range(len(lines) - 1):
        if re.match(r"(call|jmp)\s+<[^>]*>\s*$", lines[i]) and re.match(r"R_[A-Z0-9_]+\s", lines[i + 1]):
            lines[i] = re.sub(r"<([^>]*)>", r"<placeholder:\1>", lines[i])

def disasm(obj, names, src):
    ro = sh("objcopy", "-O", "binary", "--only-section=.rodata", obj, "/dev/stdout").stdout
    strs = string_labels(obj, src)
    out, keep, cur = {}, False, None
    for line in sh("objdump", "-d", "--no-show-raw-insn", "-r", obj, text=True).stdout.splitlines():
        m = re.match(r"^[0-9a-f]+ <(.*)>:$", line)
        if m:
            keep = m.group(1) in names
            cur = m.group(1)
            if keep: out[cur] = ["<%s>" % cur]
            continue
        if not keep or not line.strip(): continue
        line = re.sub(r"^\s*[0-9a-f]+:\s*", "", line)
        line = re.sub(r"\b[0-9a-f]+ <([A-Za-z_0-9.]+)(\+0x[0-9a-f]+)?>", r"<\1>", line)
        line = re.sub(r"\.rodata\+0x([0-9a-f]+)", lambda mm: ".rodata[%s]" % ro[int(mm.group(1), 16):int(mm.group(1), 16) + 16].hex(), line)
        line = re.sub(r"(\.L\.str(?:\.\d+)?)(?=[-+\s]|$)", lambda mm: "%s[%s]" % (mm.group(1), strs.get(mm.group(1), "?")), line)
        out[cur].append(line)
    for lines in out.values(): mark_call_placeholders(lines)
    return out

def label_kind(x, y):
    """The kind of label number X and Y differ in alone, or None."""
    if re.sub(r"LCPI\d+_", "LCPI_", x) == re.sub(r"LCPI\d+_", "LCPI_", y): return "constant-pool label"
    lab = lambda s: re.sub(r"\.L\.str(\.\d+)?\[", ".L.str[", s)
    if "?]" not in x + y and lab(x) == lab(y): return "string-literal label"
    return None

def classify(x, y):
    if "rodata" in x and "rodata" in y: return "rodata reference"
    ph = lambda s: re.sub(r"<placeholder:[^>]*>", "<placeholder>", s)
    if "<placeholder:" in x and ph(x) == ph(y): return "call placeholder label"
    kind = label_kind(x, y)
    if kind: return kind
    if re.match(r"mov\s+\$0x[0-9a-f]+,%(e[cd]x|r[cd]x)$", x) and re.sub(r"0x[0-9a-f]+", "N", x) == re.sub(r"0x[0-9a-f]+", "N", y):
        return "__LINE__ immediate"
    rel = lambda s: re.sub(r"-?0x[0-9a-f]+\(%rip\)", "REL(%rip)", s)
    if rel(x) == rel(y): return "rip-relative offset"
    return None

def compare(a, b):
    """The allowed differences between the lines A and B, counted by kind, and the other differing pairs."""
    kinds, bad = {}, []
    for x, y in zip(a, b):
        if x == y: continue
        k = classify(x, y)
        if k is None: bad.append((x, y))
        else: kinds[k] = kinds.get(k, 0) + 1
    return kinds, bad

def compare_by_name(a, b):
    """Compare the functions of A and B (name -> lines) one by name, so a different function order in the two
    objects (a compiler's choice) is not a difference. A name in one only, or a different length, is one."""
    kinds, bad = {}, []
    for name in sorted(set(a) | set(b)):
        if name not in a or name not in b or len(a[name]) != len(b[name]):
            bad.append(("<%s> %s" % (name, "missing in old" if name not in a else "%d lines" % len(a[name])),
                        "<%s> %s" % (name, "missing in new" if name not in b else "%d lines" % len(b[name]))))
            continue
        k, d = compare(a[name], b[name])
        for key, n in k.items(): kinds[key] = kinds.get(key, 0) + n
        bad += d
    return kinds, bad

def compile_both(bdir, old, new, tmp):
    """Compile OLD (rev:path, written next to the sources for the build) and NEW into TMP/old.o and TMP/new.o."""
    cc, fl = flags(bdir)
    root = os.path.dirname(os.path.dirname(bdir))
    oldsrc = os.path.join(root, "MagickCore", "zz_split_branch_check_old.c")
    open(oldsrc, "wb").write(sh("git", "-C", root, "show", old).stdout)
    try:
        for src, obj in ((oldsrc, "old.o"), (os.path.join(root, new), "new.o")):
            r = sh(cc, *fl, "-g0", "-DHAVE_CONFIG_H", "-I.", "-I" + root, "-c", src, "-o", os.path.join(tmp, obj), cwd=bdir, text=True)
            if r.returncode: sys.exit("compile failed: %s\n%s" % (src, r.stderr[-800:]))
    finally:
        os.remove(oldsrc)

def report(kinds, bad):
    print("allowed differences:", kinds or "none")
    if bad:
        print("DIFFERENT")
        for x, y in bad[:10]: print("  -", x, "\n  +", y)
        sys.exit(1)
    print("IDENTICAL-MODULO-LAYOUT")

def main():
    if len(sys.argv) != 4: sys.exit(__doc__)
    old, new, build = sys.argv[1:]
    root = sh("git", "rev-parse", "--show-toplevel", text=True).stdout.strip()
    bdir = os.path.join(root, "build-oracle", build)
    tmp = tempfile.mkdtemp()
    oldsrc = os.path.join(root, "MagickCore", "zz_split_branch_check_old.c")
    compile_both(bdir, old, new, tmp)
    nm = sh("nm", "--defined-only", os.path.join(tmp, "new.o"), text=True).stdout.split("\n")
    names = {l.split()[2] for l in nm if len(l.split()) == 3 and l.split()[1] in "tT"}
    a = disasm(os.path.join(tmp, "old.o"), names, oldsrc)
    b = disasm(os.path.join(tmp, "new.o"), names, os.path.join(root, new))
    la, lb = sum(map(len, a.values())), sum(map(len, b.values()))
    print("%s: %d functions, %d vs %d lines" % (build, len(names), la, lb))
    kinds, bad = compare_by_name(a, b)
    report(kinds, bad)

main()
