#!/usr/bin/env python3
"""Tests of tools/split_branch_check.py's naming of numbered static locals, run from the repository root:

    python3 tools/test_split_branch_check.py

The original object and the object of the file split off it number a function's `static` locals differently
(clang's `XMagickCommand.radius.203` is `.196` once other numbered names no longer come before it), so a
reference to the variable must compare equal across the two objects and a reference to another variable must
not. canonical_reference_names() names each such reference by its rank among the numbers of its stem that the functions use; the planted
defects below are the references that must still differ.
"""
import importlib.util, os, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("split_branch_check", os.path.join(ROOT, "tools", "split_branch_check.py"))
sbc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sbc)


def ref(sec, name, size=4096):
    return "R_X86_64_PC32\t%s[%s:%d+0]" % (sec, name, size)


def renamed(*refs):
    """The canonical lines of one function that makes the references REFS, in order."""
    return sbc.canonical_reference_names({"f": list(refs)})["f"]


class CanonicalReferenceNames(unittest.TestCase):
    OLD = [ref(".data", "F.radius.203"), ref(".data", "F.radius.212"), ref(".data", "F.factor.223"), ref(".bss", "plain", 8)]
    NEW = [ref(".data", "F.radius.196"), ref(".data", "F.radius.205"), ref(".data", "F.factor.216"), ref(".bss", "plain", 8)]

    def test_renumbered_statics_get_the_same_names(self):
        self.assertEqual(renamed(*self.OLD), renamed(*self.NEW))
        self.assertEqual(renamed(*self.OLD)[0], ref(".data", "F.radius.#0"))
        self.assertEqual(renamed(*self.OLD)[1], ref(".data", "F.radius.#1"))

    def test_swapped_order_differs(self):
        # planted defect: the two radius variables are referenced the other way round
        swapped = [self.NEW[1], self.NEW[0]] + self.NEW[2:]
        self.assertNotEqual(renamed(*self.OLD), renamed(*swapped))

    def test_a_reference_to_another_stem_differs(self):
        # planted defect: the variable the old object calls F.factor.* is called F.radius.* in the new one
        other = self.NEW[:2] + [ref(".data", "F.radius.216")] + self.NEW[3:]
        self.assertNotEqual(renamed(*self.OLD), renamed(*other))

    def test_a_missing_variable_differs(self):
        # planted defect: the second radius variable is never referenced (it is gone from the new object)
        fewer = [ref(".data", "F.radius.196"), ref(".data", "F.radius.196"), self.NEW[2], self.NEW[3]]
        self.assertNotEqual(renamed(*self.OLD), renamed(*fewer))

    def test_ranks_count_only_the_variables_the_functions_reference(self):
        # gcc's `display.N` has no function in its name: the kept file's one variable is rank 0 in both objects even
        # when the original object holds another `display.M` that only the moved functions use (not referenced here)
        self.assertEqual(renamed(ref(".bss", "display.4915", 8)), renamed(ref(".bss", "display.4863", 8)))

    def test_plain_names_other_sections_and_other_references_are_untouched(self):
        self.assertEqual(renamed(ref(".bss", "counter", 4), "mov %eax,%ebx", ref(".data.rel.local", "t.7", 8)),
                         [ref(".bss", "counter", 4), "mov %eax,%ebx", ref(".data.rel.local", "t.#0", 8)])


if __name__ == "__main__":
    unittest.main()
