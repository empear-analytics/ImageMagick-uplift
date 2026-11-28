#!/usr/bin/env python3
"""Tests of tools/reuse_check.py's returned-output form on a small synthetic file, run from the
repository root:

    python3 tools/test_reuse_check.py

The fixture has the shape of accelerate.c's worker cleanups: a block that destroys the image it
built when the work did not finish, `if ((ok == MagickFalse) && (image != (Image *) NULL))
image=DestroyImage(image);`, and an existing helper that takes the image by value (a parameter of
pointer type, the pointer itself copied) and returns it (Recipe E step 3). Replacing the block by
`image=Helper(ok,image);` must print OK, as it does for a helper whose parameter is a scalar;
each planted defect must be refused: the result assigned to another variable, another variable
passed for the image, the helper ending without `return(image);`, the arguments transposed, and
the block's test edited on the way.
"""
import contextlib
import io
import os
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import reuse_check as R  # noqa: E402

HELPER = """static Image *DestroyUnfinished(const MagickBooleanType ok,
  Image *image)
{
  if ((ok == MagickFalse) && (image != (Image *) NULL))
    image=DestroyImage(image);
  return(image);
}

"""

WORK = """static Image *Work(const Image *source)
{
  Image
    *image,
    *other;

  MagickBooleanType
    ok;

  image=Clone(source);
  other=Clone(source);
  ok=Run(image,other);
  if ((ok == MagickFalse) && (image != (Image *) NULL))
    image=DestroyImage(image);
  return(image);
}
"""

OLD = HELPER + WORK

GOOD = "  image=DestroyUnfinished(ok,image);\n"
BLOCK = ("  if ((ok == MagickFalse) && (image != (Image *) NULL))\n"
         "    image=DestroyImage(image);\n")


def run(new, old=None, helper="DestroyUnfinished"):
    """reuse_check on fixture.c in a fresh repository: OLD committed, NEW in the tree."""
    old = OLD if old is None else old
    here = os.getcwd()
    with tempfile.TemporaryDirectory() as d:
        os.chdir(d)
        try:
            env = dict(os.environ, GIT_AUTHOR_NAME="t", GIT_AUTHOR_EMAIL="t@t",
                       GIT_COMMITTER_NAME="t", GIT_COMMITTER_EMAIL="t@t")
            subprocess.run(["git", "init", "-q"], check=True, env=env)
            with open("fixture.c", "w") as f:
                f.write(old)
            subprocess.run(["git", "add", "fixture.c"], check=True, env=env)
            subprocess.run(["git", "-c", "core.hooksPath=/dev/null", "commit", "-q", "-m",
                            "old"], check=True, env=env)
            with open("fixture.c", "w") as f:
                f.write(new)
            out = io.StringIO()
            rc = 0
            with contextlib.redirect_stdout(out):
                try:
                    rc = R.main(["reuse_check.py", "fixture.c", helper])
                except SystemExit as e:  # the script refuses some steps by SystemExit
                    print(e)
                    rc = 1
            return rc, out.getvalue()
        finally:
            os.chdir(here)


class ReturnedPointerOutput(unittest.TestCase):
    def check(self, call, helper=HELPER):
        return run(helper + WORK.replace(BLOCK, call))

    def test_ok(self):
        rc, out = self.check(GOOD)
        self.assertEqual(rc, 0, out)
        self.assertIn("return(image) assigned back to image", out)

    def test_assigned_to_another_variable(self):
        rc, out = self.check("  other=DestroyUnfinished(ok,image);\n")
        self.assertNotEqual(rc, 0, out)

    def test_another_variable_passed(self):
        rc, out = self.check("  image=DestroyUnfinished(ok,other);\n")
        self.assertNotEqual(rc, 0, out)

    def test_helper_without_return(self):
        rc, out = self.check(GOOD, helper=HELPER.replace("  return(image);\n", "  return(NULL);\n"))
        self.assertNotEqual(rc, 0, out)

    def test_arguments_transposed(self):
        rc, out = self.check("  image=DestroyUnfinished(image,ok);\n")
        self.assertNotEqual(rc, 0, out)

    def test_block_edited(self):
        rc, out = run(HELPER + WORK.replace(BLOCK, GOOD).replace(
            "  ok=Run(image,other);", "  ok=Run(other,image);"))
        self.assertNotEqual(rc, 0, out)


MEMBER_HELPER = """static size_t PadWidth(const Image *image,size_t width)
{
  if (image->columns != image->rows)
    width=image->rows;
  return(width);
}

"""

MEMBER_WORK = """static size_t Work(const Source *source)
{
  Info
    info;

  info.width=source->image->columns;
  info.height=source->image->rows;
  if (source->image->columns != source->image->rows)
    info.width=source->image->rows;
  return(info.width+info.height);
}
"""

MEMBER_BLOCK = ("  if (source->image->columns != source->image->rows)\n"
                "    info.width=source->image->rows;\n")


class ReturnedMemberOutput(unittest.TestCase):
    """The output is a member of a local struct object: `info.width=PadWidth(...,info.width);`."""

    def check(self, call):
        return run(MEMBER_HELPER + MEMBER_WORK.replace(MEMBER_BLOCK, call),
                   old=MEMBER_HELPER + MEMBER_WORK, helper="PadWidth")

    def test_ok(self):
        rc, out = self.check("  info.width=PadWidth(source->image,info.width);\n")
        self.assertEqual(rc, 0, out)
        self.assertIn("return(width) assigned back to info.width", out)

    def test_assigned_to_another_member(self):
        rc, out = self.check("  info.height=PadWidth(source->image,info.width);\n")
        self.assertNotEqual(rc, 0, out)

    def test_another_member_passed(self):
        rc, out = self.check("  info.width=PadWidth(source->image,info.height);\n")
        self.assertNotEqual(rc, 0, out)

    def test_through_a_pointer(self):
        rc, out = self.check("  source->width=PadWidth(source->image,source->width);\n")
        self.assertNotEqual(rc, 0, out)


POINTER_WORK = """static size_t Work(Source *source)
{
  source->width=source->image->columns;
  if (source->image->columns != source->image->rows)
    source->width=source->image->rows;
  return(source->width);
}
"""

POINTER_BLOCK = ("  if (source->image->columns != source->image->rows)\n"
                 "    source->width=source->image->rows;\n")


class ReturnedMemberThroughPointer(unittest.TestCase):
    """The block really writes `source->width`: the call is the block's body under the renaming,
    so the only reason left to refuse it is the `->` of the target."""

    def test_member_through_a_pointer_refused(self):
        rc, out = run(MEMBER_HELPER + POINTER_WORK.replace(
            POINTER_BLOCK, "  source->width=PadWidth(source->image,source->width);\n"),
            old=MEMBER_HELPER + POINTER_WORK, helper="PadWidth")
        self.assertNotEqual(rc, 0, out)
        # not read as the returned-output form, so the helper's return(width) stays unmatched
        self.assertIn("return(width)", out)

    def test_same_block_through_a_local_copy_accepted(self):
        """Control: the same block on a struct local is accepted, so the refusal above is the `->`."""
        work = POINTER_WORK.replace("Source *source)\n{\n", "Source *source)\n{\n  Info\n    info;\n\n")
        work = work.replace("source->width", "info.width")
        block = POINTER_BLOCK.replace("source->width", "info.width")
        rc, out = run(MEMBER_HELPER + work.replace(
            block, "  info.width=PadWidth(source->image,info.width);\n"),
            old=MEMBER_HELPER + work, helper="PadWidth")
        self.assertEqual(rc, 0, out)



def run_with_header(new, header, new_header=None):
    """reuse_check on fixture.c, which includes lib/fixture-private.h holding HEADER: both
    committed with fixture.c as OLD (the work function alone), NEW in the tree, and the
    header as NEW_HEADER in the tree when given."""
    here = os.getcwd()
    with tempfile.TemporaryDirectory() as d:
        os.chdir(d)
        try:
            env = dict(os.environ, GIT_AUTHOR_NAME="t", GIT_AUTHOR_EMAIL="t@t",
                       GIT_COMMITTER_NAME="t", GIT_COMMITTER_EMAIL="t@t")
            subprocess.run(["git", "init", "-q"], check=True, env=env)
            os.mkdir("lib")
            with open("lib/fixture-private.h", "w") as f:
                f.write(header)
            with open("fixture.c", "w") as f:
                f.write(INCLUDE + WORK)
            subprocess.run(["git", "add", "fixture.c", "lib/fixture-private.h"], check=True,
                           env=env)
            subprocess.run(["git", "-c", "core.hooksPath=/dev/null", "commit", "-q", "-m",
                            "old"], check=True, env=env)
            with open("fixture.c", "w") as f:
                f.write(new)
            if new_header is not None:
                with open("lib/fixture-private.h", "w") as f:
                    f.write(new_header)
            out = io.StringIO()
            rc = 0
            with contextlib.redirect_stdout(out):
                try:
                    rc = R.main(["reuse_check.py", "fixture.c", "DestroyUnfinished"])
                except SystemExit as e:
                    print(e)
                    rc = 1
            return rc, out.getvalue()
        finally:
            os.chdir(here)


INCLUDE = '#include "lib/fixture-private.h"\n\n'
HEADER = HELPER.replace("static Image *", "static inline Image *")


class HelperInIncludedHeader(unittest.TestCase):
    """2026-11-28: the helper is a `static inline` function of a private header that the
    file includes directly (accelerate-unsharp.c and accelerate-core-private.h's
    DestroyUnfinishedImage)."""

    def test_ok(self):
        rc, out = run_with_header(INCLUDE + WORK.replace(BLOCK, GOOD), HEADER)
        self.assertEqual(rc, 0, out)
        self.assertIn("defined in lib/fixture-private.h", out)

    def test_header_not_included(self):
        rc, out = run_with_header(WORK.replace(BLOCK, GOOD), HEADER)
        self.assertNotEqual(rc, 0, out)

    def test_header_changed(self):
        rc, out = run_with_header(INCLUDE + WORK.replace(BLOCK, GOOD), HEADER,
                                  HEADER.replace("image=DestroyImage(image);",
                                                 "image=(Image *) NULL;"))
        self.assertNotEqual(rc, 0, out)

    def test_block_edited(self):
        rc, out = run_with_header(INCLUDE + WORK.replace(BLOCK, GOOD).replace(
            "ok=Run(image,other);", "ok=Run(other,image);"), HEADER)
        self.assertNotEqual(rc, 0, out)

    def test_not_inline_in_header(self):
        rc, out = run_with_header(INCLUDE + WORK.replace(BLOCK, GOOD), HELPER)
        self.assertNotEqual(rc, 0, out)

if __name__ == "__main__":
    unittest.main()
