#!/usr/bin/env python3
"""Tests of tools/register_source.py on small synthetic build files, run from the repository root:

    python3 tools/test_register_source.py

The fixture's Makefile.in has the shapes automake writes: an object list whose first entry stands on the
`am__objects_1 = ` line (as accelerate.lo does), a middle entry, a last entry without a continuation, a
dependency list, and the sources list. Registering a file next to the list's first entry must give a
tab-indented continuation line, not a second `am__objects_1 = ` assignment (the defect that broke the
wide build in campaign A round 27); a middle and a last entry must keep working as before.
"""
import os
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOL = os.path.join(ROOT, "tools", "register_source.py")
P = "MagickCore/libMagickCore_@MAGICK_MAJOR_VERSION@_@MAGICK_ABI_SUFFIX@_la-"

MAKEFILE_IN = (
    "am__objects_1 = " + P + "first.lo \\\n"
    "\t" + P + "middle.lo \\\n"
    "\t" + P + "last.lo\n"
    "am__depfiles_remade = ./$(DEPDIR)/dummy.Po \\\n"
    "\tMagickCore/$(DEPDIR)/" + P[len("MagickCore/"):] + "first.Plo \\\n"
    "\tMagickCore/$(DEPDIR)/" + P[len("MagickCore/"):] + "middle.Plo\n"
    "MAGICKCORE_BASE_SRCS = \\\n"
    "  MagickCore/first.c \\\n"
    "  MagickCore/middle.c \\\n"
    "  MagickCore/last.c\n"
)
MAKEFILE_AM = "MAGICKCORE_BASE_SRCS = \\\n  MagickCore/first.c \\\n  MagickCore/middle.c \\\n  MagickCore/last.c \\\n"


def register(orig, new):
    d = tempfile.mkdtemp()
    os.makedirs(os.path.join(d, "MagickCore"))
    os.makedirs(os.path.join(d, "scripts"))
    open(os.path.join(d, "Makefile.in"), "w").write(MAKEFILE_IN)
    open(os.path.join(d, "MagickCore", "Makefile.am"), "w").write(MAKEFILE_AM)
    open(os.path.join(d, "MagickCore", "Make.com"), "w").write("")
    open(os.path.join(d, "scripts", "format_c_api_docs"), "w").write("")
    r = subprocess.run([sys.executable, TOOL, orig, new], cwd=d, capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    return open(os.path.join(d, "Makefile.in")).read()


class RegisterSourceTest(unittest.TestCase):
    def test_first_entry_of_object_list(self):
        s = register("first", "first-part")
        self.assertEqual(s.count("am__objects_1 = "), 1)
        self.assertIn("am__objects_1 = " + P + "first.lo \\\n\t" + P + "first-part.lo \\\n\t" + P + "middle.lo",
                      s)
        self.assertIn("  MagickCore/first.c \\\n  MagickCore/first-part.c \\\n", s)

    def test_middle_entry(self):
        s = register("middle", "middle-part")
        self.assertIn("\t" + P + "middle.lo \\\n\t" + P + "middle-part.lo \\\n\t" + P + "last.lo\n", s)
        self.assertEqual(s.count("am__objects_1 = "), 1)

    def test_last_entry(self):
        s = register("last", "last-part")
        self.assertIn("\t" + P + "last.lo \\\n\t" + P + "last-part.lo\nam__depfiles_remade", s)


if __name__ == "__main__":
    unittest.main()
