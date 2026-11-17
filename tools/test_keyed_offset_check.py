#!/usr/bin/env python3
"""Tests of tools/keyed_offset_check.py on a small synthetic file, run from the repository root:

    python3 tools/test_keyed_offset_check.py

The fixture is the shape of display-image.c's `XSelectMagnifyDigit`: a `switch` of two runs of labels,
each run assigning the key minus its own base, and `default: break;`. The correct rewrite (a table of
`{ label, base }` rows and one loop) must print OK; each planted defect, a lost label, a doubled label,
a row with the other run's base, a swapped pair of rows, a loop that stores another expression, a
different controlling expression, a changed assignment target, a wrong typedef, a statement edited
elsewhere in the file and an old `switch` whose groups disagree, must be refused.
"""
import os
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOL = os.path.join(ROOT, "tools", "keyed_offset_check.py")

OLD = """
static int Other(int a)
{
  return(a+1);
}

static void SelectDigit(Windows *windows,const KeySym key_symbol)
{
  switch ((int) key_symbol)
  {
    case K_0:
    case K_1:
    case K_2:
    {
      windows->magnify.data=(key_symbol-K_0);
      break;
    }
    case KP_0:
    case KP_1:
    {
      windows->magnify.data=(key_symbol-KP_0);
      break;
    }
    default:
      break;
  }
}
"""

NEW = """
static int Other(int a)
{
  return(a+1);
}

typedef struct _DigitKey
{
  int
    key,
    base;
} DigitKey;

static const DigitKey
  DigitKeys[] =
  {
    { K_0, K_0 },
    { K_1, K_0 },
    { K_2, K_0 },
    { KP_0, KP_0 },
    { KP_1, KP_0 }
  };

static void SelectDigit(Windows *windows,const KeySym key_symbol)
{
  const int
    key = (int) key_symbol;

  size_t
    i;

  for (i=0; i < (sizeof(DigitKeys)/sizeof(*DigitKeys)); i++)
    if (DigitKeys[i].key == key)
      {
        windows->magnify.data=(key_symbol-DigitKeys[i].base);
        break;
      }
}
"""


def run(old, new):
    with tempfile.TemporaryDirectory() as d:
        po, pn = os.path.join(d, "old.c"), os.path.join(d, "new.c")
        with open(po, "w") as f:
            f.write(old)
        with open(pn, "w") as f:
            f.write(new)
        r = subprocess.run([sys.executable, TOOL, pn, "SelectDigit", "DigitKeys", "--old", po],
                           capture_output=True, text=True)
        return r.returncode, r.stdout + r.stderr


class KeyedOffsetCheck(unittest.TestCase):
    def refused(self, old, new, fragment=""):
        code, out = run(old, new)
        self.assertNotEqual(code, 0, out)
        self.assertIn("FAIL", out)
        self.assertIn(fragment, out)

    def test_correct_step_is_accepted(self):
        code, out = run(OLD, NEW)
        self.assertEqual(code, 0, out)
        self.assertTrue(out.startswith("OK SelectDigit: 5 labels in 2 groups"), out)

    def test_lost_label(self):
        self.refused(OLD, NEW.replace("    { K_2, K_0 },\n", ""), "table rows")

    def test_doubled_label(self):
        self.refused(OLD, NEW.replace("    { K_2, K_0 },\n", "    { K_2, K_0 },\n    { K_2, K_0 },\n"), "table rows")

    def test_other_runs_base(self):
        self.refused(OLD, NEW.replace("{ KP_1, KP_0 }", "{ KP_1, K_0 }"), "table rows")

    def test_swapped_rows(self):
        self.refused(OLD, NEW.replace("    { K_1, K_0 },\n    { K_2, K_0 },\n", "    { K_2, K_0 },\n    { K_1, K_0 },\n"),
                     "table rows")

    def test_other_stored_expression(self):
        self.refused(OLD, NEW.replace("(key_symbol-DigitKeys[i].base)", "(key_symbol+DigitKeys[i].base)"), "loop")

    def test_other_controlling_expression(self):
        self.refused(OLD, NEW.replace("key = (int) key_symbol;", "key = (int) windows->x;"), "loop")

    def test_other_target(self):
        self.refused(OLD, NEW.replace("windows->magnify.data=(key_symbol-Dig", "windows->magnify.size=(key_symbol-Dig"),
                     "loop")

    def test_wrong_typedef(self):
        self.refused(OLD, NEW.replace("    key,\n    base;", "    key,\n    base,\n    extra;"), "typedef")

    def test_edit_elsewhere(self):
        self.refused(OLD, NEW.replace("return(a+1);", "return(a+2);"), "outside")

    def test_old_groups_disagree(self):
        self.refused(OLD.replace("windows->magnify.data=(key_symbol-KP_0)", "windows->magnify.size=(key_symbol-KP_0)"),
                     NEW, "share one V")

    def test_old_group_without_break(self):
        self.refused(OLD.replace("      windows->magnify.data=(key_symbol-KP_0);\n      break;\n",
                                 "      windows->magnify.data=(key_symbol-KP_0);\n"), NEW, "break")


if __name__ == "__main__":
    unittest.main()
