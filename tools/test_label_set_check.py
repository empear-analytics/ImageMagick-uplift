#!/usr/bin/env python3
"""Tests of tools/label_set_check.py on a small synthetic file, run from the repository root:

    python3 tools/test_label_set_check.py

The fixture has the shape of display-image.c's `XImageWindowCommand`: a `switch` of one group
that returns (`case 10:`), a group of two labels whose body returns on every path, and `default:`.
The correct step, which moves the two-label group behind the `switch` under a predicate, must print
OK; each planted defect, a lost label, an added label, a doubled label, a changed statement of the
moved body, a changed statement of the kept group, a changed statement outside the function, a
predicate away from its caller, a predicate that was there before, a wrong argument, a moved body
holding a `break`, a kept group that falls out of the `switch`, and an argument written in the
`switch`, must be refused.
"""
import os
import sys
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import label_set_check as L  # noqa: E402

OLD = """
static int Before(int a)
{
  return(a+1);
}

static int Caller(int key,int state)
{
  int
    x;

  x=1;
  switch ((int) key)
  {
    case 10:
    {
      return(1);
    }
    case 20:
    case 21:
    {
      if (state != 0)
        return(2);
      x++;
      return(3);
    }
    default:
      break;
  }
  return(x);
}
"""

PRED = """
static MagickBooleanType IsTwenty(const KeySym key_symbol)
{
  switch ((int) key_symbol)
  {
    case 20:
    case 21:
      return(MagickTrue);
    default:
      break;
  }
  return(MagickFalse);
}

"""

NEW_CALLER = """static int Caller(int key,int state)
{
  int
    x;

  x=1;
  switch ((int) key)
  {
    case 10:
    {
      return(1);
    }
    default:
      break;
  }
  if (IsTwenty(key) != MagickFalse)
    {
      if (state != 0)
        return(2);
      x++;
      return(3);
    }
  return(x);
}
"""

HEAD = OLD[:OLD.index("static int Caller")]
NEW = HEAD + PRED + NEW_CALLER


def run(new, old=OLD, pred="IsTwenty"):
    return L.check(old, new, "Caller", pred)


class LabelSetCheck(unittest.TestCase):
    def refused(self, new, **kw):
        with self.assertRaises(L.Refused):
            run(new, **kw)

    def test_correct_step(self):
        self.assertIn("2 labels", run(NEW))

    def test_lost_label(self):
        self.refused(NEW.replace("    case 21:\n", ""))

    def test_added_label(self):
        self.refused(NEW.replace("    case 21:\n", "    case 21:\n    case 22:\n"))

    def test_doubled_label(self):
        self.refused(NEW.replace("    case 21:\n", "    case 21:\n    case 21:\n"))

    def test_moved_body_changed(self):
        self.refused(NEW.replace("      x++;\n      return(3);", "      x--;\n      return(3);"))

    def test_kept_group_changed(self):
        self.refused(NEW.replace("      return(1);", "      return(5);"))

    def test_statement_outside_changed(self):
        self.refused(NEW.replace("return(a+1);", "return(a+2);"))

    def test_predicate_away_from_caller(self):
        self.refused(NEW.replace(PRED, "").replace("static int Before", PRED + "static int Before"))

    def test_predicate_existed(self):
        self.refused(NEW, old=HEAD + PRED + OLD[OLD.index("static int Caller"):])

    def test_wrong_argument(self):
        self.refused(NEW.replace("IsTwenty(key)", "IsTwenty(state)"))

    def test_break_in_moved_body(self):
        old = OLD.replace("      x++;\n      return(3);", "      x++;\n      if (x > 3)\n        break;\n      return(3);")
        new = NEW.replace("      x++;\n      return(3);", "      x++;\n      if (x > 3)\n        break;\n      return(3);")
        self.refused(new, old=old)

    def test_kept_group_falls_out(self):
        old = OLD.replace("      return(1);", "      x=4;")
        new = NEW.replace("      return(1);", "      x=4;")
        self.refused(new, old=old)

    def test_argument_written_in_switch(self):
        old = OLD.replace("      return(1);", "      key=3;\n      return(1);")
        new = NEW.replace("      return(1);", "      key=3;\n      return(1);")
        self.refused(new, old=old)

    def test_predicate_not_static(self):
        self.refused(NEW.replace("static MagickBooleanType IsTwenty", "MagickBooleanType IsTwenty"))

    def test_no_default_in_predicate(self):
        self.refused(NEW.replace("    default:\n      break;\n  }\n  return(MagickFalse);", "  }\n  return(MagickFalse);"))

    def test_break_ending_the_moved_group_is_dropped(self):
        old = OLD.replace("      x++;\n      return(3);", "      x++;\n      break;").replace(
            "      if (state != 0)\n        return(2);\n", "")
        new = NEW.replace("      x++;\n      return(3);", "      x++;").replace(
            "      if (state != 0)\n        return(2);\n", "")
        self.assertIn("2 labels", run(new, old=old))


if __name__ == "__main__":
    unittest.main(verbosity=1)
