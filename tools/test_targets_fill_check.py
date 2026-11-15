#!/usr/bin/env python3
"""Tests of tools/targets_fill_check.py on a small synthetic file, run from the repository root:

    python3 tools/test_targets_fill_check.py

The fixture has the shape of accelerate.c's OpenCL workers: a head that acquires a device and
a buffer, each followed by `goto cleanup;`, moved by Recipe I into a helper that writes them
through a targets struct. The correct step must print OK in both of Recipe I's fill forms:
the assignments directly before the call, and step 2's `const` initialiser wrapped over two
lines; in both, `git diff` keeps the caller's `goto cleanup;` as unchanged context, paired
with the block's last exit. Each planted defect must be refused: an initialiser out of member
order, a fill from another variable, a binding of another member, a written variable left
without its `*`, an exit dropped from the helper, an extra `return(MagickFalse);`, a caller
exit that is not the block's (`goto other;`), a statement added outside the helper, a
moved statement edited on the way, and a caller exit dropped while another, unchanged function
of the file still calls the helper followed by the same exit (that function's exit lends
nothing).
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
import targets_fill_check as T  # noqa: E402

OLD = """static int Work(Image *image,ExceptionInfo *exception)
{
  Device
    device;

  Buffer
    buffer;

  int
    status;

  status=0;
  device=AcquireDevice(image);
  if (device == (Device) NULL)
    goto cleanup;
  buffer=AcquireBuffer(image,device,exception);
  if (buffer == (Buffer) NULL)
    goto cleanup;
  status=Run(device,buffer);
cleanup:
  Release(device,buffer);
  return(status);
}
"""

TYPEDEF = """typedef struct _WorkTargets
{
  Device
    *device;

  Buffer
    *buffer;
} WorkTargets;

"""

HELPER = """static MagickBooleanType AcquireWork(Image *image,ExceptionInfo *exception,
  const WorkTargets *targets)
{
  Device
    *const device = targets->device;

  Buffer
    *const buffer = targets->buffer;

  *device=AcquireDevice(image);
  if (*device == (Device) NULL)
    return(MagickFalse);
  *buffer=AcquireBuffer(image,*device,exception);
  if (*buffer == (Buffer) NULL)
    return(MagickFalse);
  return(MagickTrue);
}

"""

CALLER_FILL = """static int Work(Image *image,ExceptionInfo *exception)
{
  Device
    device;

  Buffer
    buffer;

  int
    status;

  WorkTargets
    targets;

  status=0;
  targets.device=(&device);
  targets.buffer=(&buffer);
  if (AcquireWork(image,exception,&targets) == MagickFalse)
    goto cleanup;
  status=Run(device,buffer);
cleanup:
  Release(device,buffer);
  return(status);
}
"""

CALLER_CONST = """static int Work(Image *image,ExceptionInfo *exception)
{
  Device
    device;

  Buffer
    buffer;

  int
    status;

  const WorkTargets
    targets = { &device,
      &buffer };

  status=0;
  if (AcquireWork(image,exception,&targets) == MagickFalse)
    goto cleanup;
  status=Run(device,buffer);
cleanup:
  Release(device,buffer);
  return(status);
}
"""


OTHER = """static int Other(Image *image,ExceptionInfo *exception)
{
  WorkTargets
    targets;

  if (AcquireWork(image,exception,&targets) == MagickFalse)
    goto cleanup;
cleanup:
  return(0);
}

"""


def run(new, old=OLD):
    """targets_fill_check on fixture.c in a fresh repository: OLD committed, NEW in the tree."""
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
            with contextlib.redirect_stdout(out):
                rc = T.main(["targets_fill_check.py", "fixture.c", "WorkTargets", "targets",
                             "AcquireWork"])
            return rc, out.getvalue()
        finally:
            os.chdir(here)


class TargetsFillCheck(unittest.TestCase):
    def ok(self, new):
        rc, out = run(new)
        self.assertEqual(rc, 0, out)
        self.assertIn("OK WorkTargets", out)

    def refused(self, new):
        rc, out = run(new)
        self.assertEqual(rc, 1, out)
        self.assertIn("FAIL", out)

    def test_fill_form(self):
        self.ok(TYPEDEF + HELPER + CALLER_FILL)

    def test_const_form_wrapped(self):
        self.ok(TYPEDEF + HELPER + CALLER_CONST)

    def test_initialiser_out_of_order(self):
        self.refused(TYPEDEF + HELPER + CALLER_CONST.replace(
            "{ &device,\n      &buffer }", "{ &buffer,\n      &device }"))

    def test_fill_from_another_variable(self):
        self.refused(TYPEDEF + HELPER + CALLER_FILL.replace(
            "targets.buffer=(&buffer);", "targets.buffer=(&status);"))

    def test_binding_of_another_member(self):
        self.refused(TYPEDEF + HELPER.replace("*const buffer = targets->buffer;",
                                              "*const buffer = targets->device;")
                     + CALLER_FILL)

    def test_missing_star(self):
        self.refused(TYPEDEF + HELPER.replace("AcquireBuffer(image,*device,exception)",
                                              "AcquireBuffer(image,device,exception)")
                     + CALLER_FILL)

    def test_exit_dropped(self):
        self.refused(TYPEDEF + HELPER.replace(
            "  if (*buffer == (Buffer) NULL)\n    return(MagickFalse);\n",
            "  if (*buffer == (Buffer) NULL)\n    ;\n") + CALLER_FILL)

    def test_extra_return(self):
        self.refused(TYPEDEF + HELPER.replace(
            "  return(MagickTrue);\n}", "  if (*buffer == (Buffer) NULL)\n"
            "    return(MagickFalse);\n  return(MagickTrue);\n}") + CALLER_FILL)

    def test_other_caller_exit(self):
        self.refused(TYPEDEF + HELPER + CALLER_FILL.replace(
            "MagickFalse)\n    goto cleanup;", "MagickFalse)\n    goto other;"))

    def test_statement_added_outside(self):
        self.refused(TYPEDEF + HELPER + CALLER_FILL.replace(
            "  status=Run(device,buffer);", "  status=1;\n  status=Run(device,buffer);"))

    def test_moved_statement_edited(self):
        self.refused(TYPEDEF + HELPER.replace("AcquireDevice(image)", "AcquireDevice(NULL)")
                     + CALLER_FILL)

    def test_unchanged_caller_lends_no_exit(self):
        rc, out = run(OTHER + TYPEDEF + HELPER + CALLER_FILL, old=OTHER + OLD)
        self.assertEqual(rc, 0, out)
        rc, out = run(OTHER + TYPEDEF + HELPER + CALLER_FILL.replace(
            "MagickFalse)\n    goto cleanup;\n  status=Run", "MagickFalse)\n  status=Run"),
            old=OTHER + OLD)
        self.assertEqual(rc, 1, out)
        self.assertIn("exit(s) left the block", out)


if __name__ == "__main__":
    unittest.main()
