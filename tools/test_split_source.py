#!/usr/bin/env python3
"""Tests of tools/split_source.py on small synthetic files, run from the repository root:

    python3 tools/test_split_source.py

Each case writes a C file whose licence header is MagickCore/xwindow.c's, splits it with the tool and
compiles both halves (gcc -fsyntax-only -Wall -Werror) with and without the delegate macro. The cases
that need a conditional carried check that the new file opens and closes it once, that the original keeps
its own, and that both halves compile in both configurations; the refusal cases check what the tool has
always refused (a shared static, an #elif branch, two conditionals) and one planted defect: the new file
with its #endif removed must not compile, so the compile check can see the failure that carrying the
conditional prevents.
"""
import os, subprocess, sys, tempfile, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOL = os.path.join(ROOT, "tools", "split_source.py")
HEAD = open(os.path.join(ROOT, "MagickCore", "xwindow.c")).read().split("\n")
HEAD = "\n".join(HEAD[:HEAD.index("*/") + 1])
HEADER_H = "#define MagickExport\nint FamilyRoot(int);\nint FamilyRoot2(int);\nint KeptRoot(int);\n"
INCLUDES = "\n/*\n  Include declarations.\n*/\n#include \"synth.h\"\n"

FAMILY = """
static int FamilyHelper(int x)
{
  return(x+1);
}

MagickExport int FamilyRoot(int x)
{
  return(FamilyHelper(x));
}
"""
KEPT = """
MagickExport int KeptRoot(int x)
{
  return(x*2);
}
"""


class SplitSource(unittest.TestCase):
    def split(self, body, roots=("FamilyRoot",), extra_h=""):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = self.tmp.name
        self.addCleanup(self.tmp.cleanup)
        open(os.path.join(self.dir, "synth.h"), "w").write(HEADER_H + extra_h)
        self.src = os.path.join(self.dir, "synth.c")
        open(self.src, "w").write(HEAD + "\n" + INCLUDES + body)
        self.out = os.path.join(self.dir, "out")
        os.mkdir(self.out)
        r = subprocess.run([sys.executable, TOOL, self.src, os.path.join(self.dir, "synth-family.c"),
                            "Synth Family Methods", *roots, "--out", self.out],
                           cwd=ROOT, capture_output=True, text=True)
        return r

    def compiles(self, name, *defs):
        r = subprocess.run(["gcc", "-fsyntax-only", "-Wall", "-Werror", "-I", self.dir, *defs,
                            os.path.join(self.out, name)], capture_output=True, text=True)
        return r.returncode == 0, r.stderr

    def assert_both_compile(self):
        for name in ("synth.c", "synth-family.c"):
            for defs in ((), ("-DDELEGATE",)):
                ok, err = self.compiles(name, *defs)
                self.assertTrue(ok, "%s %s: %s" % (name, defs, err))

    def test_plain_file_is_unchanged(self):
        r = self.split(FAMILY + KEPT)
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        new = open(os.path.join(self.out, "synth-family.c")).read()
        self.assertNotIn("#if", new)
        self.assertNotIn("conditionals carried", r.stdout)
        self.assert_both_compile()

    def test_wrapped_family_carries_the_conditional(self):
        r = self.split("#if defined(DELEGATE)\n" + FAMILY + KEPT + "#else\n" + KEPT.replace("x*2", "0") + "#endif\n")
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        self.assertIn("conditionals carried", r.stdout)
        new = open(os.path.join(self.out, "synth-family.c")).read()
        old = open(os.path.join(self.out, "synth.c")).read()
        self.assertEqual(new.count("#if defined(DELEGATE)"), 1)
        self.assertEqual(new.count("#endif"), 1)
        self.assertNotIn("#else", new)
        self.assertTrue(new.rstrip().endswith("#endif"))
        self.assertEqual(old.count("#if defined(DELEGATE)"), 1)
        self.assertEqual(old.count("#else"), 1)
        self.assertNotIn("FamilyRoot", old)
        self.assert_both_compile()

    def test_conditional_in_the_include_block_is_not_opened_twice(self):
        # the #if stands before the last #include: the copied include block holds it
        r = self.split("#if defined(DELEGATE)\n#include <stdio.h>\n" + FAMILY + KEPT + "#endif\n")
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        new = open(os.path.join(self.out, "synth-family.c")).read()
        self.assertEqual(new.count("#if defined(DELEGATE)"), 1)
        self.assertEqual(new.count("#endif"), 1)
        self.assert_both_compile()

    def test_forward_declaration_of_a_pointer_returning_static_moves(self):
        fam = ("static const char\n  *FamilyName(const int);\n"
               "\nstatic const char *FamilyName(const int x)\n{\n  return(x == 0 ? \"a\" : \"b\");\n}\n"
               + FAMILY.replace("return(FamilyHelper(x));", "return(FamilyHelper(x)+(FamilyName(x)[0] == 0));"))
        r = self.split("#if defined(DELEGATE)\n" + fam + KEPT + "#endif\n")
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        new = open(os.path.join(self.out, "synth-family.c")).read()
        old = open(os.path.join(self.out, "synth.c")).read()
        self.assertEqual(new.count("*FamilyName(const int);"), 1)
        self.assertNotIn("FamilyName", old)
        self.assert_both_compile()

    def test_stub_of_a_moved_root_moves_under_the_else_line(self):
        stub = "\nMagickExport int FamilyRoot(int x)\n{\n  return(-1);\n}\n"
        r = self.split("#if defined(DELEGATE)\n" + FAMILY + "#else" + stub + "#endif\n" + KEPT)
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        new = open(os.path.join(self.out, "synth-family.c")).read()
        old = open(os.path.join(self.out, "synth.c")).read()
        self.assertEqual(new.count("#else"), 1)
        self.assertEqual(new.count("return(-1);"), 1)
        self.assertNotIn("return(-1);", old)
        self.assert_both_compile()

    def test_original_keeps_its_else_when_the_stub_moves_and_another_stub_stays(self):
        stub = "\nMagickExport int FamilyRoot(int x)\n{\n  return(-1);\n}\n"
        kept_stub = KEPT.replace("x*2", "0")
        r = self.split("#if defined(DELEGATE)\n" + FAMILY + "#else" + stub + kept_stub + "#endif\n")
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        old = open(os.path.join(self.out, "synth.c")).read()
        new = open(os.path.join(self.out, "synth-family.c")).read()
        self.assertEqual(old.count("#else"), 1)
        self.assertEqual(old.count("MagickExport int KeptRoot"), 1)
        self.assertEqual(new.count("#else"), 1)
        self.assertNotIn("KeptRoot", new)
        self.assertNotIn("FamilyRoot", old)
        self.assert_both_compile()

    def test_planted_defect_unclosed_conditional_does_not_compile(self):
        r = self.split("#if defined(DELEGATE)\n" + FAMILY + KEPT + "#endif\n")
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        path = os.path.join(self.out, "synth-family.c")
        text = open(path).read()
        open(path, "w").write(text[:text.rindex("#endif")])
        ok, _ = self.compiles("synth-family.c", "-DDELEGATE")
        self.assertFalse(ok)
        open(path, "w").write(text.replace("#if defined(DELEGATE)\n", ""))
        ok, _ = self.compiles("synth-family.c", "-DDELEGATE")
        self.assertFalse(ok)

    def test_shared_static_still_refused(self):
        r = self.split("#if defined(DELEGATE)\n" + FAMILY + KEPT.replace("x*2", "FamilyHelper(x)") + "#endif\n")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("shared statics", r.stderr)

    def test_elif_branch_refused(self):
        r = self.split("#if defined(OTHER)\n" + KEPT + "#elif defined(DELEGATE)\n" + FAMILY + "#endif\n")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("#elif", r.stderr)

    def test_family_in_two_conditionals_refused(self):
        family2 = "\nMagickExport int FamilyRoot2(int x)\n{\n  return(x-1);\n}\n"
        r = self.split("#if defined(DELEGATE)\n" + FAMILY + "#endif\n" + KEPT + "#if defined(OTHER)\n" + family2 + "#endif\n",
                       roots=("FamilyRoot", "FamilyRoot2"))
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("different conditionals", r.stderr)


if __name__ == "__main__":
    unittest.main()
