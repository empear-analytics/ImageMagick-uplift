#!/usr/bin/env python3
"""Planted-defect tests for tools/split_source.py (run from the repository root).

    python3 tools/test_split_source_regions.py

Each case writes a small original under a temporary directory, runs the splitter on it with `--out`, and checks what
came out or what it refused. The accepted case holds what the tool takes since 2026-11-12 (a family inside a file-level
`#if`, a two-line forward declaration between moved functions, an `extern "C"` pair around a callback, a table and an
enumeration at the top of the file that only the family uses); every planted defect must still be refused.
"""
import os, subprocess, sys, tempfile

HDR = "".join(open("MagickCore/animate.c").readlines()[:31]).rstrip("\n").split("*/")[0] + "*/\n"

ORIGINAL = HDR + r'''
/*
  Include declarations.
*/
#include "MagickCore/studio.h"

#if defined(MAGICKCORE_X11_DELEGATE)
/*
  Enumeration declarations.
*/
typedef enum
{
  FamilyOne,
  FamilyTwo
} FamilyChoice;

/*
  Constant declarations.
*/
static const char
  FamilyText[] =
  {
    "family"
  };

static const char
  KeptText[] =
  {
    "kept"
  };

MagickExport int Kept(void)
{
  return((int) KeptText[0]);
}

static void Callee(int *p)
{
  (*p)+=(int) FamilyText[0];
}

@BETWEEN@
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int Pred(int x)
{
  return(Late(x));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static int Late(int x)
{
  FamilyChoice
    choice;

  choice=FamilyTwo;
  return(x+(int) choice);
}

MagickExport int FamilyRoot(int x)
{
  int
    v;

  v=Pred(x);
  Callee(&v);
  return(v+(int) FamilyOne);
}
@TAIL@#else
MagickExport int Kept(void)
{
  return(0);
}
#endif
'''

FWD = "static int\n  Late(int);\n"


def fill(between, tail=""):
    return ORIGINAL.replace("@BETWEEN@", between).replace("@TAIL@", tail)


def run(text, root="FamilyRoot"):
    with tempfile.TemporaryDirectory() as d:
        src, out = os.path.join(d, "animate.c"), os.path.join(d, "out")
        os.mkdir(out)
        open(src, "w").write(text)
        p = subprocess.run([sys.executable, "tools/split_source.py", src, "MagickCore/animate-window.c", "Title", root,
                            "--out", out], capture_output=True, text=True)
        files = {}
        for n in os.listdir(out):
            files[n] = open(os.path.join(out, n)).read()
        return p.returncode, p.stdout + p.stderr, files


def check(name, ok):
    print("%-4s %s" % ("ok" if ok else "FAIL", name))
    return ok


def main():
    good = fill(FWD)
    results = []
    rc, msg, files = run(good)
    new, kept = files.get("animate-window.c", ""), files.get("animate.c", "")
    results.append(check("accepted: exit status 0", rc == 0))
    results.append(check("accepted: the new file opens the conditional before its declarations",
                         "#include \"MagickCore/studio.h\"\n#if defined(MAGICKCORE_X11_DELEGATE)\n\n/*\n  Typedef declarations." in new))
    results.append(check("accepted: the new file closes it", new.rstrip("\n").endswith("#endif")))
    results.append(check("accepted: enumeration, table and forward declaration moved",
                         all(s in new for s in ("} FamilyChoice;", "FamilyText[] =", FWD, 'extern "C" {'))))
    results.append(check("accepted: the kept file lost them and keeps the shared table",
                         "FamilyChoice" not in kept and "FamilyText" not in kept and "KeptText[] =" in kept
                         and "Late" not in kept and "FamilyRoot" not in kept and "MagickExport int Kept" in kept))
    results.append(check("accepted: the empty banners went", "Enumeration declarations." not in kept))
    # planted defects
    kept2 = fill(FWD + "\nstatic int\n  Kept2(int);\n").replace(
        "static void Callee(int *p)", "static int Kept2(int x)\n{\n  return(x);\n}\n\nstatic void Callee(int *p)", 1)
    rc, msg, _ = run(kept2)
    results.append(check("refused: a forward declaration of a kept static between moved functions",
                         rc != 0 and "refused" in msg))
    shared = good.replace("return((int) KeptText[0]);", "return((int) KeptText[0]+(int) FamilyText[0]);")
    rc, msg, _ = run(shared)
    results.append(check("refused: a table both parts use", rc != 0 and "FamilyText" in msg))
    elsewhere = good.replace("MagickExport int Kept(void)\n{\n  return((int) KeptText[0]);",
        "static int Elsewhere(int x)\n{\n  return(x);\n}\n\nMagickExport int Kept(void)\n{\n  return((int) KeptText[0]+Elsewhere(1));", 1
        ).replace("  Callee(&v);", "  Callee(&v);\n  v+=Elsewhere(v);")
    rc, msg, _ = run(elsewhere)
    results.append(check("refused: a static both parts call", rc != 0 and "shared" in msg))
    middle = good.replace("static int Late(int x)", "#else\nstatic int Late(int x)", 1).replace(
        "MagickExport int FamilyRoot", "#endif\nMagickExport int FamilyRoot", 1)
    rc, msg, _ = run(middle)
    results.append(check("refused: an #else between the moved functions", rc != 0))
    split_c = good.replace("#if defined(__cplusplus) || defined(c_plusplus)\n}\n#endif\n", "", 1)
    rc, msg, _ = run(split_c)
    results.append(check("refused: an extern \"C\" block that never closes", rc != 0))
    print("%d of %d checks passed" % (sum(results), len(results)))
    sys.exit(0 if all(results) else 1)


if __name__ == "__main__":
    main()
