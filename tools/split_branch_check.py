#!/usr/bin/env python3
"""split_branch_check.py OLDREV:OLDFILE NEWFILE BUILD

Recipe S, *Preprocessor branches* exception of 2026-11-11: a split may move an `#if defined(...)` branch
verbatim with the function that holds it when a build that compiles that branch shows the moved functions'
instructions unchanged. This compiles the original file as it stood at OLDREV and the new file as it is in
the working tree, both with the flags of build-oracle/BUILD (cand-wide for X11/OpenCL, ci-gcc-min or
ci-gcc-dlg for the non-HDRI branches), at -g0, disassembles every function the new file defines in both
objects, and compares them function by function, matched by name (so the order the compiler laid the
functions out in does not matter), line by line. Allowed differences, each counted in the output:
  - a `__LINE__` immediate (`mov $N,%ecx` / `%edx`) shifted by a constant per function,
  - a .rodata reference (resolved to the 16 bytes it points at; `__FILE__` strings differ),
  - a compiler constant-pool label number (.LCPI<n>_<m>), a rip-relative offset,
  - a string-literal label number (clang's .L.str.<n>), when both labels hold the same bytes: each
    reference is resolved to the NUL-terminated string the label points at (the path the object was
    compiled from reads as __FILE__ in both), so a changed literal still prints DIFFERENT.
  - a `.data.rel.ro` reference (a table of string pointers a `switch` was turned into): the offset in the
    section differs when other tables stand before it, so each reference is resolved to the strings the
    consecutive pointer slots from that offset point at (to the end of the run of slots), and a changed
    string or slot count still prints DIFFERENT,
  - a `.bss` or `.data` reference (a function's `static` variable): the offset in the section differs when
    other variables stand before it, so each reference is resolved to the symbol (name and size) it points
    into, plus the offset inside it, and a reference to another variable still prints DIFFERENT,
  - the label objdump prints for a `call` or `jmp` whose target a relocation names (the line after it, which
    is compared): the placeholder address resolves to whatever symbol stands at it in each object,
  - the address of a function taken from another object: `lea F(%rip),%r` with an R_X86_64_PC32 relocation
    when F stands in the same object, `mov F@GOTPCREL(%rip),%r` with an R_X86_64_REX_GOTPCRELX relocation
    when it does not (the moved code takes the address of XError, which stays in the original); both are
    read as the lea form and the symbol and addend are compared,
and the alignment padding after a function's last instruction (nop forms), which depends on what follows
the function in its object, is not compared.
Prints IDENTICAL-MODULO-LAYOUT or DIFFERENT (with the first differing lines) and exits 0 or 1.
"""
import os, re, subprocess, sys, tempfile

def sh(*a, **k):
    return subprocess.run(a, capture_output=True, **k)

def flags(build):
    mk = open(os.path.join(build, "Makefile")).read()
    get = lambda n: re.search(r"^%s = (.*)$" % n, mk, re.M).group(1)
    # -Werror only decides whether a diagnostic stops the compile; it does not change the code generated, and the X
    # trees' flags turn upstream -Wunused-parameter diagnostics of the original file into errors (2026-11-12)
    return get("CC"), [f for f in get("CPPFLAGS").split() + get("CFLAGS").split() if f != "-Werror"]

def section_bytes(obj, sec):
    return sh("objcopy", "-O", "binary", "--only-section=" + sec, obj, "/dev/stdout").stdout

def string_labels(obj, src):
    """Map each local string label (.L.str.<n>) of OBJ to the hex of the NUL-terminated bytes it points at."""
    symbols = re.findall(r"(?m)^([0-9a-f]+) .{7} (\S+)\s+[0-9a-f]+ (\.L\.str(?:\.\d+)?)$", sh("objdump", "-t", obj, text=True).stdout)
    sections = {sec: section_bytes(obj, sec) for _, sec, _ in symbols}
    texts = {label: sections[sec][int(at, 16):].split(b"\0", 1)[0] for at, sec, label in symbols}
    return {label: "__FILE__" if text == src.encode() else text.hex() for label, text in texts.items()}

def pointer_slots(obj, strs, src):
    """Map each slot offset of .data.rel.ro of OBJ to what its 64-bit relocation points at: the hex of the
    NUL-terminated string (a string-literal label or a .rodata.str section), else symbol and addend."""
    sections = {}
    slots = {}
    for off, sym, add in re.findall(r"(?m)^([0-9a-f]+) R_X86_64_64\s+(\S+?)\+0x([0-9a-f]+)$",
                                    sh("objdump", "-r", "-j", ".data.rel.ro", obj, text=True).stdout):
        if sym.startswith(".rodata.str"):
            if sym not in sections: sections[sym] = section_bytes(obj, sym)
            text = sections[sym][int(add, 16):].split(b"\0", 1)[0]
            slots[int(off, 16)] = "__FILE__" if text == src.encode() else text.hex()
        elif sym.startswith(".L.str"): slots[int(off, 16)] = strs.get(sym, "?")
        else: slots[int(off, 16)] = "%s+%s" % (sym, add)
    return slots

def data_symbols(obj):
    """Map a section (.bss, .data) of OBJ to its (value, size, name) object symbols."""
    syms = {}
    for value, sec, size, name in re.findall(r"(?m)^([0-9a-f]+) .{7} (\.bss|\.data)\s+([0-9a-f]+) (\S+)$",
                                             sh("objdump", "-t", obj, text=True).stdout):
        if name != sec: syms.setdefault(sec, []).append((int(value, 16), int(size, 16), name))
    return syms

def symbol_at(syms, sec, target):
    """`sec[name:size+offset]` for the symbol of section SEC that the address TARGET (in the section) falls in, or None."""
    for value, size, name in syms.get(sec, []):
        if value <= target < value + size: return "%s[%s:%d+%d]" % (sec, name, size, target - value)
    return None

def table_at(slots, off):
    """The run of resolved pointer slots of SLOTS from offset OFF to the first gap, joined, or None."""
    run = []
    while off in slots: run.append(slots[off]); off += 8
    return ",".join(run) if run else None

def drop_padding(lines):
    """LINES without the alignment padding (nop forms) after the last instruction."""
    while len(lines) > 1 and re.match(r"^(cs |data16 )*(nop[wlq]?\b|xchg\s+%ax,%ax)", lines[-1]): lines = lines[:-1]
    return lines

def disasm(obj, names, src):
    ro = sh("objcopy", "-O", "binary", "--only-section=.rodata", obj, "/dev/stdout").stdout
    strs = string_labels(obj, src)
    slots = pointer_slots(obj, strs, src)
    syms = data_symbols(obj)
    out, at, keep, cur = {}, {}, False, None
    for line in sh("objdump", "-d", "--no-show-raw-insn", "-r", obj, text=True).stdout.splitlines():
        m = re.match(r"^[0-9a-f]+ <(.*)>:$", line)
        if m:
            keep = m.group(1) in names
            cur = m.group(1)
            if keep: out[cur] = ["<%s>" % cur]; at[cur] = [None]
            continue
        if not keep or not line.strip(): continue
        addr = int(re.match(r"^\s*([0-9a-f]+):", line).group(1), 16)
        line = re.sub(r"^\s*[0-9a-f]+:\s*", "", line)
        line = re.sub(r"\b[0-9a-f]+ <([A-Za-z_0-9.]+)(\+0x[0-9a-f]+)?>", r"<\1>", line)
        line = re.sub(r"\.rodata\+0x([0-9a-f]+)", lambda mm: ".rodata[%s]" % ro[int(mm.group(1), 16):int(mm.group(1), 16) + 16].hex(), line)
        line = re.sub(r"(\.L\.str(?:\.\d+)?)(?=[-+\s]|$)", lambda mm: "%s[%s]" % (mm.group(1), strs.get(mm.group(1), "?")), line)
        def resolve(mm):
            table = table_at(slots, int(mm.group(2), 16) * (1 if mm.group(1) == "+" else -1) + 4)
            return mm.group(0) if table is None else ".data.rel.ro[%s]" % table
        line = re.sub(r"\.data\.rel\.ro([+-])0x([0-9a-f]+)", resolve, line)
        out[cur].append(line); at[cur].append(addr)
    for name, lines in out.items():
        for k in range(len(lines) - 1):
            # a PC-relative reference to a variable: the target is the addend plus 4 plus the bytes of any
            # immediate between the displacement and the end of the instruction, found from the addresses
            m = re.match(r"^R_X86_64_PC32\s+(\.bss|\.data)(?:([+-])0x([0-9a-f]+))?\s*$", lines[k])
            if m:
                nxt = next((at[name][j] for j in range(k + 1, len(lines)) if not lines[j].startswith("R_X86_64_")), None)
                addend = int(m.group(3) or "0", 16) * (-1 if m.group(2) == "-" else 1)
                ref = symbol_at(syms, m.group(1), addend + 4 + (0 if nxt is None else nxt - at[name][k] - 4))
                if ref: lines[k] = "R_X86_64_PC32\t" + ref
            if re.match(r"^(call|jmp)\s+<", lines[k]) and lines[k + 1].startswith("R_X86_64_"):
                lines[k] = re.sub(r"<.*>", "<RELOC>", lines[k])
            if re.match(r"^mov\s+0x0\(%rip\),%\w+", lines[k]) and re.match(r"^R_X86_64_REX_GOTPCRELX\s", lines[k + 1]):
                lines[k] = re.sub(r"^mov(\s+)", r"lea\1", lines[k])
                lines[k + 1] = lines[k + 1].replace("R_X86_64_REX_GOTPCRELX", "R_X86_64_PC32", 1)
    return {name: drop_padding(lines) for name, lines in out.items()}

def label_kind(x, y):
    """The kind of label number X and Y differ in alone, or None."""
    if re.sub(r"LCPI\d+_", "LCPI_", x) == re.sub(r"LCPI\d+_", "LCPI_", y): return "constant-pool label"
    lab = lambda s: re.sub(r"\.L\.str(\.\d+)?\[", ".L.str[", s)
    if "?]" not in x + y and lab(x) == lab(y): return "string-literal label"
    return None

def classify(x, y):
    if "rodata" in x and "rodata" in y: return "rodata reference"
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
