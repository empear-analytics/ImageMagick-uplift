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
    def split(self, body, roots=("FamilyRoot",), extra_h="", objects_h=False):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = self.tmp.name
        self.addCleanup(self.tmp.cleanup)
        open(os.path.join(self.dir, "synth.h"), "w").write(HEADER_H + extra_h)
        if objects_h: open(os.path.join(self.dir, "objs.h"), "w").write("const char *kObjTable = \"x\";\n")
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

    def test_enumeration_between_the_moved_functions_moves_and_a_shared_one_refuses(self):
        enum = "\ntypedef enum\n{\n  FamilyOne,\n  FamilyTwo\n} FamilyChoice;\n"
        fam = FAMILY.replace("return(FamilyHelper(x));", "return(FamilyHelper(x)+(int) FamilyTwo);")
        helper, root = fam.split("\nMagickExport int FamilyRoot", 1)
        r = self.split(helper + enum + "\nMagickExport int FamilyRoot" + root + KEPT)
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        new = open(os.path.join(self.out, "synth-family.c")).read()
        old = open(os.path.join(self.out, "synth.c")).read()
        self.assertEqual(new.count("} FamilyChoice;"), 1)
        self.assertNotIn("FamilyChoice", old)
        self.assert_both_compile()
        # planted defect: the kept root names the enumeration too, so it may not move
        kept = KEPT.replace("return(x*2);", "return(x*2+(int) FamilyOne);")
        r = self.split(helper + enum + "\nMagickExport int FamilyRoot" + root + kept)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("FamilyOne, an enumerator", r.stderr + r.stdout)

    def test_inline_static_helper_is_a_static(self):
        # `inline static T Name(...)` is a static: it moves with its only caller, and a kept caller makes it shared
        fam = FAMILY.replace("static int FamilyHelper", "inline static int FamilyHelper")
        r = self.split(fam + KEPT)
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        new = open(os.path.join(self.out, "synth-family.c")).read()
        old = open(os.path.join(self.out, "synth.c")).read()
        self.assertEqual(new.count("inline static int FamilyHelper"), 1)
        self.assertNotIn("FamilyHelper", old)
        self.assert_both_compile()
        # planted defect: a kept function calls the helper as well
        r = self.split(fam + KEPT.replace("return(x*2);", "return(FamilyHelper(x));"))
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("FamilyHelper", r.stderr + r.stdout)

    def test_forward_declaration_with_the_name_in_column_zero_moves(self):
        # `static T` / `Name(...);` (no indent), above the first moved function and between moved functions
        top = "static int\nFamilyHelper(int);\n\n"
        r = self.split(top + FAMILY + KEPT)
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        new = open(os.path.join(self.out, "synth-family.c")).read()
        old = open(os.path.join(self.out, "synth.c")).read()
        self.assertEqual(new.count("FamilyHelper(int);"), 1)
        self.assertNotIn("FamilyHelper", old)
        self.assert_both_compile()
        helper, root = FAMILY.split("\nMagickExport int FamilyRoot", 1)
        r = self.split(helper + "\nstatic int\nFamilyHelper(int);\n" + "\nMagickExport int FamilyRoot" + root + KEPT)
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        self.assertNotIn("FamilyHelper", open(os.path.join(self.out, "synth.c")).read())
        # planted defect: the kept root calls the helper too, so the declaration may not move
        r = self.split(top + FAMILY + KEPT.replace("return(x*2);", "return(FamilyHelper(x));"))
        self.assertNotEqual(r.returncode, 0)

    def test_extern_c_pair_above_the_first_function_moves_with_its_comment(self):
        opener = "#if defined(__cplusplus) || defined(c_plusplus)\nextern \"C\" {\n#endif\n"
        closer = "#if defined(__cplusplus) || defined(c_plusplus)\n}\n#endif\n"
        callback = "static int FamilyCallback(int x)\n{\n  return(x+2);\n}\n\n"
        root = "MagickExport int FamilyRoot(int x)\n{\n  return(FamilyCallback(x));\n}\n\n"
        body = "/*\n  FamilyCallback() adds two.\n*/\n" + opener + "\n" + callback + root + closer + KEPT
        r = self.split(body)
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        new = open(os.path.join(self.out, "synth-family.c")).read()
        old = open(os.path.join(self.out, "synth.c")).read()
        self.assertEqual(new.count('extern "C" {'), 1)
        self.assertIn("FamilyCallback() adds two.", new)
        self.assertNotIn("extern", old)
        self.assertNotIn("FamilyCallback", old)
        self.assert_both_compile()
        # planted defect: a kept function stands between the moved ones and the closer, so the pair would be left open
        body = "/*\n  FamilyCallback() adds two.\n*/\n" + opener + "\n" + callback + KEPT + "\n" + root + closer
        r = self.split(body)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("extern", r.stderr + r.stdout)

    def test_multiline_defines_that_only_the_family_names_move_with_it(self):
        macros = ("\n/*\n  Define declarations.\n*/\n#define FamilyScale(x) \\\n  ((x)*2+ \\\n   FamilyBias(x))\n"
                  "#define FamilyBias(x) \\\n  ((x)+ \\\n   1)\n#define FamilyOne  1\n\n")
        fam = FAMILY.replace("return(x+1);", "return(FamilyScale(x)+FamilyOne);")
        r = self.split(macros + fam + KEPT)
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        new = open(os.path.join(self.out, "synth-family.c")).read()
        old = open(os.path.join(self.out, "synth.c")).read()
        for name in ("#define FamilyScale(x) \\\n  ((x)*2+ \\\n   FamilyBias(x))", "#define FamilyBias(x) \\\n  ((x)+ \\\n   1)",
                     "#define FamilyOne  1"):
            self.assertEqual(new.count(name), 1, name)
        for name in ("FamilyScale", "FamilyBias", "FamilyOne"):
            self.assertNotIn(name, old)
        self.assert_both_compile()
        # planted defects: the kept root names the outer macro, or the inner one the outer macro names
        for use in ("FamilyScale(x)", "FamilyBias(x)"):
            r = self.split(macros + fam + KEPT.replace("return(x*2);", "return(%s);" % use))
            self.assertNotEqual(r.returncode, 0, use)
            self.assertIn("refused", r.stderr + r.stdout)

    def test_variables_that_only_the_family_names_move_with_it(self):
        # one declaration of two mutable static arrays between the moved functions, and a mutable static scalar at the top
        top = "static int\n  FamilyCount = 0;\n\n"
        group = ("static int\n  FamilyTable[] =\n  {\n    1, 2\n  },\n  FamilyOffsets[] =\n  {\n    3\n  };\n\n")
        helper, root = FAMILY.split("\nMagickExport int FamilyRoot", 1)
        root = root.replace("return(FamilyHelper(x));",
                            "FamilyCount++;\n  return(FamilyHelper(x)+FamilyTable[0]+FamilyOffsets[0]);")
        body = top + helper + "\n" + group + "MagickExport int FamilyRoot" + root + KEPT
        r = self.split(body)
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        new = open(os.path.join(self.out, "synth-family.c")).read()
        old = open(os.path.join(self.out, "synth.c")).read()
        for name in ("FamilyCount = 0;", "FamilyTable[] =", "FamilyOffsets[] ="):
            self.assertEqual(new.count(name), 1, name)
        for name in ("FamilyCount", "FamilyTable", "FamilyOffsets"):
            self.assertNotIn(name, old)
        self.assert_both_compile()
        # planted defects: the kept root names the scalar, or one declarator of the group, so the split is refused
        for use in ("FamilyCount", "FamilyOffsets[0]"):
            r = self.split(body.replace("return(x*2);", "return(x*2+%s);" % use))
            self.assertNotEqual(r.returncode, 0, use)
            self.assertIn("refused", r.stderr + r.stdout)

    def test_grouped_forward_declarations_in_an_extern_c_block_split(self):
        # stream.c's pixel cache interfaces: one `static T` group declares a kept and a moved static, another
        # wraps its parameter list over two lines, all inside an `extern "C"` block
        ext_open = "#if defined(__cplusplus) || defined(c_plusplus)\nextern \"C\" {\n#endif\n"
        ext_close = "#if defined(__cplusplus) || defined(c_plusplus)\n}\n#endif\n"
        decls = ("\n/*\n  Declare pixel cache interfaces.\n*/\n" + ext_open + "\nstatic int\n  KeptHelper(int),\n  FamilyHelper2(int);\n\n"
                 "static const char\n  *FamilyName(const int,const int,\n    const int);\n\n" + ext_close)
        fam = ("\nMagickExport int FamilyRoot(int x)\n{\n  return(FamilyHelper2(x)+(FamilyName(x,x,x)[0] == 0));\n}\n"
               "\nstatic int FamilyHelper2(int x)\n{\n  return(x+1);\n}\n"
               "\nstatic const char *FamilyName(const int x,const int y,const int z)\n{\n"
               "  return(x+y+z == 0 ? \"a\" : \"b\");\n}\n")
        kept = ("\nMagickExport int KeptRoot(int x)\n{\n  return(KeptHelper(x));\n}\n"
                "\nstatic int KeptHelper(int x)\n{\n  return(x*2);\n}\n")
        r = self.split(decls + fam + kept)
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        new = open(os.path.join(self.out, "synth-family.c")).read()
        old = open(os.path.join(self.out, "synth.c")).read()
        self.assertIn("static int\n  KeptHelper(int);\n", old)
        self.assertNotIn("FamilyHelper2", old)
        self.assertNotIn("FamilyName", old)
        self.assertEqual(old.count('extern "C" {'), 1)
        self.assertIn("static int\n  FamilyHelper2(int);\n", new)
        self.assertIn("  *FamilyName(const int,const int,\n    const int);\n", new)
        self.assertEqual(new.count('extern "C" {'), 1)
        self.assertLess(new.index('extern "C" {'), new.index("FamilyHelper2(int);"))
        self.assertLess(new.index("const int);"), new.index("\n}\n#endif"))
        self.assertNotIn("KeptHelper", new)
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

    def test_header_that_defines_objects_stays_in_the_original(self):
        r = self.split('#include "objs.h"\n' + FAMILY + KEPT.replace("x*2", "(int) kObjTable[0]"), objects_h=True)
        self.assertEqual(r.returncode, 0, r.stderr + r.stdout)
        self.assertIn("objs.h defines objects", r.stdout)
        old = open(os.path.join(self.out, "synth.c")).read()
        new = open(os.path.join(self.out, "synth-family.c")).read()
        self.assertIn('#include "objs.h"', old)
        self.assertNotIn("objs.h", new)
        self.assertIn('#include "synth.h"', new)

    def test_header_that_defines_objects_refused_when_the_family_names_one(self):
        fam = FAMILY.replace("return(FamilyHelper(x));", "return(FamilyHelper(x)+kObjTable[0]);")
        r = self.split('#include "objs.h"\n' + fam + KEPT, objects_h=True)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("kObjTable", r.stderr)

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
