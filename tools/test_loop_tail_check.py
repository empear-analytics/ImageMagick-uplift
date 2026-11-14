#!/usr/bin/env python3
"""Tests of tools/loop_tail_check.py on a small synthetic file, run from the repository root:

    python3 tools/test_loop_tail_check.py

The fixture has the shape of display-image.c's `XLoadDirectoryImages`: a `for` loop whose body ends
with a progress block that `break`s when the monitor refuses, and a helper returning whether it
broke. The correct step must print OK (also when the old block ends in an explicit `continue;`);
each planted defect, a changed statement of the moved block, a swapped return constant, an exit
turned into the wrong constant, two transposed arguments, a computed argument, a call that is not the
last statement of its block, a helper with five parameters, a helper that already existed, a helper
away from its caller, an edit outside the function, a `#pragma` and a `goto` in the helper, must be
refused.
"""
import os
import sys
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import loop_tail_check as L  # noqa: E402

OLD = """
static int Before(int a)
{
  return(a+1);
}

static Image *Load(Image *images,const int count)
{
  int
    i;

  for (i=0; i < count; i++)
  {
    Append(&images,i);
    if (images->monitor != (Monitor) NULL)
      {
        MagickBooleanType
          proceed;

        proceed=Progress(images,i,count);
        if (proceed == MagickFalse)
          break;
      }
  }
  return(images);
}
"""

HELPER = """static MagickBooleanType IsCancelled(Image *images,const int i,
  const int count)
{
  if (images->monitor != (Monitor) NULL)
    {
      MagickBooleanType
        proceed;

      proceed=Progress(images,i,count);
      if (proceed == MagickFalse)
        return(MagickTrue);
    }
  return(MagickFalse);
}

"""

HEAD = OLD[:OLD.index("static Image *Load")]

NEW = HEAD + HELPER + """static Image *Load(Image *images,const int count)
{
  int
    i;

  for (i=0; i < count; i++)
  {
    Append(&images,i);
    if (IsCancelled(images,i,count) != MagickFalse)
      break;
  }
  return(images);
}
"""


def run(new, old=OLD):
    return L.check(old, new, "Load", "IsCancelled")


class LoopTailCheck(unittest.TestCase):
    def refused(self, new, **kw):
        with self.assertRaises(L.Refused):
            run(new, **kw)

    def test_correct_step(self):
        self.assertIn("3 parameters", run(NEW))

    def test_old_block_ends_in_continue(self):
        old = OLD.replace("          break;\n      }\n", "          break;\n        continue;\n      }\n")
        new = NEW.replace("        return(MagickTrue);\n    }\n", "        return(MagickTrue);\n      return(MagickFalse);\n    }\n")
        self.assertIn("3 parameters", run(new, old=old))

    def test_moved_statement_changed(self):
        self.refused(NEW.replace("Progress(images,i,count)", "Progress(images,i,count+1)"))

    def test_swapped_constant(self):
        self.refused(NEW.replace("return(MagickTrue);", "return(MagickFalse);", 1))

    def test_exit_becomes_wrong_constant(self):
        self.refused(NEW.replace("  return(MagickFalse);\n}\n\nstatic Image", "  return(MagickTrue);\n}\n\nstatic Image"))

    def test_transposed_arguments(self):
        self.refused(NEW.replace("IsCancelled(images,i,count)", "IsCancelled(images,count,i)"))

    def test_computed_argument(self):
        self.refused(NEW.replace("IsCancelled(images,i,count)", "IsCancelled(images,i,count+0)"))

    def test_call_not_last_statement(self):
        self.refused(NEW.replace("      break;\n  }", "      break;\n    Append(&images,0);\n  }"))

    def test_five_parameters(self):
        old = OLD.replace("(Image *images,const int count)", "(Image *images,const int count)")
        new = NEW.replace("const int count)\n{\n  if (images", "const int count,int a,int b)\n{\n  if (images")
        self.refused(new.replace("IsCancelled(images,i,count)", "IsCancelled(images,i,count,a,b)"), old=old)

    def test_helper_existed(self):
        self.refused(NEW, old=NEW)

    def test_helper_away_from_caller(self):
        self.refused(NEW.replace(HELPER, "").replace("static int Before", HELPER + "static int Before"))

    def test_edit_outside(self):
        self.refused(NEW.replace("return(a+1);", "return(a+2);"))

    def test_pragma_in_helper(self):
        self.refused(NEW.replace("      MagickBooleanType\n        proceed;", "#pragma omp critical\n      MagickBooleanType\n        proceed;"))

    def test_goto_in_helper(self):
        self.refused(NEW.replace("      proceed=Progress(images,i,count);", "      goto done;\n      proceed=Progress(images,i,count);"))


if __name__ == "__main__":
    unittest.main(verbosity=1)
