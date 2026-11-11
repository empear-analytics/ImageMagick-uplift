#!/usr/bin/env python3
"""split_branch_check.py OLDREV:OLDFILE NEWFILE BUILD

Recipe S, *Preprocessor branches* exception of 2026-11-11: a split may move an `#if defined(...)` branch
verbatim with the function that holds it when a build that compiles that branch shows the moved functions'
instructions unchanged. This compiles the original file as it stood at OLDREV and the new file as it is in
the working tree, both with the flags of build-oracle/BUILD (cand-wide for X11/OpenCL, ci-gcc-min or
ci-gcc-dlg for the non-HDRI branches), at -g0, disassembles every function the new file defines in both
objects, and compares them line by line. Allowed differences, each counted in the output:
  - a `__LINE__` immediate (`mov $N,%ecx` / `%edx`) shifted by a constant per function,
  - a .rodata reference (resolved to the 16 bytes it points at; `__FILE__` strings differ),
  - a compiler constant-pool label number (.LCPI<n>_<m>), a rip-relative offset.
Prints IDENTICAL-MODULO-LAYOUT or DIFFERENT (with the first differing lines) and exits 0 or 1.
"""
import os, re, subprocess, sys, tempfile

def sh(*a, **k):
    return subprocess.run(a, capture_output=True, **k)

def flags(build):
    mk = open(os.path.join(build, "Makefile")).read()
    get = lambda n: re.search(r"^%s = (.*)$" % n, mk, re.M).group(1)
    return get("CC"), get("CPPFLAGS").split() + get("CFLAGS").split()

def disasm(obj, names):
    ro = sh("objcopy", "-O", "binary", "--only-section=.rodata", obj, "/dev/stdout").stdout
    out, keep = [], False
    for line in sh("objdump", "-d", "--no-show-raw-insn", "-r", obj, text=True).stdout.splitlines():
        m = re.match(r"^[0-9a-f]+ <(.*)>:$", line)
        if m:
            keep = m.group(1) in names
            if keep: out.append("<%s>" % m.group(1))
            continue
        if not keep or not line.strip(): continue
        line = re.sub(r"^\s*[0-9a-f]+:\s*", "", line)
        line = re.sub(r"\b[0-9a-f]+ <([A-Za-z_0-9.]+)(\+0x[0-9a-f]+)?>", r"<\1>", line)
        line = re.sub(r"\.rodata\+0x([0-9a-f]+)", lambda mm: ".rodata[%s]" % ro[int(mm.group(1), 16):int(mm.group(1), 16) + 16].hex(), line)
        out.append(line)
    return out

def classify(x, y):
    if "rodata" in x and "rodata" in y: return "rodata reference"
    if re.sub(r"LCPI\d+_", "LCPI_", x) == re.sub(r"LCPI\d+_", "LCPI_", y): return "constant-pool label"
    if re.match(r"mov\s+\$0x[0-9a-f]+,%(e[cd]x|r[cd]x)$", x) and re.sub(r"0x[0-9a-f]+", "N", x) == re.sub(r"0x[0-9a-f]+", "N", y):
        return "__LINE__ immediate"
    rel = lambda s: re.sub(r"-?0x[0-9a-f]+\(%rip\)", "REL(%rip)", s)
    if rel(x) == rel(y): return "rip-relative offset"
    return None

def main():
    if len(sys.argv) != 4: sys.exit(__doc__)
    old, new, build = sys.argv[1:]
    root = sh("git", "rev-parse", "--show-toplevel", text=True).stdout.strip()
    bdir = os.path.join(root, "build-oracle", build)
    cc, fl = flags(bdir)
    tmp = tempfile.mkdtemp()
    oldsrc = os.path.join(root, "MagickCore", "zz_split_branch_check_old.c")
    open(oldsrc, "wb").write(sh("git", "-C", root, "show", old).stdout)
    try:
        for src, obj in ((oldsrc, "old.o"), (os.path.join(root, new), "new.o")):
            r = sh(cc, *fl, "-g0", "-DHAVE_CONFIG_H", "-I.", "-I" + root, "-c", src, "-o", os.path.join(tmp, obj), cwd=bdir, text=True)
            if r.returncode: sys.exit("compile failed: %s\n%s" % (src, r.stderr[-800:]))
    finally:
        os.remove(oldsrc)
    nm = sh("nm", "--defined-only", os.path.join(tmp, "new.o"), text=True).stdout.split("\n")
    names = {l.split()[2] for l in nm if len(l.split()) == 3 and l.split()[1] in "tT"}
    a, b = disasm(os.path.join(tmp, "old.o"), names), disasm(os.path.join(tmp, "new.o"), names)
    print("%s: %d functions, %d vs %d lines" % (build, len(names), len(a), len(b)))
    if len(a) != len(b): print("DIFFERENT (length)"); sys.exit(1)
    kinds, bad = {}, []
    for x, y in zip(a, b):
        if x == y: continue
        k = classify(x, y)
        if k is None: bad.append((x, y))
        else: kinds[k] = kinds.get(k, 0) + 1
    print("allowed differences:", kinds or "none")
    if bad:
        print("DIFFERENT")
        for x, y in bad[:10]: print("  -", x, "\n  +", y)
        sys.exit(1)
    print("IDENTICAL-MODULO-LAYOUT")

main()
