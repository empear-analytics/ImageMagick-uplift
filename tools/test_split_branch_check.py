#!/usr/bin/env python3
"""Tests of tools/split_branch_check.py's naming of numbered static locals, run from the repository root:

    python3 tools/test_split_branch_check.py

The original object and the object of the file split off it number a function's `static` locals differently
(clang's `XMagickCommand.radius.203` is `.196` once other numbered names no longer come before it), so a
reference to the variable must compare equal across the two objects and a reference to another variable must
not. canonical_data_names() names each such symbol by its rank among the numbers of its stem; the planted
defects below are the references that must still differ.
"""
import importlib.util, os, unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("split_branch_check", os.path.join(ROOT, "tools", "split_branch_check.py"))
sbc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sbc)


def names(rows):
    """The canonical names of ROWS ((value, size, name) in .data), in order."""
    return [n for _, _, n in sbc.canonical_data_names({".data": rows})[".data"]]


class CanonicalDataNames(unittest.TestCase):
    OLD = [(0, 4096, "F.radius.203"), (4096, 4096, "F.radius.212"), (8192, 4096, "F.factor.223"), (12288, 8, "plain")]
    NEW = [(0, 4096, "F.radius.196"), (4096, 4096, "F.radius.205"), (8192, 4096, "F.factor.216"), (12288, 8, "plain")]

    def test_renumbered_statics_get_the_same_names(self):
        self.assertEqual(names(self.OLD), names(self.NEW))
        self.assertEqual(names(self.OLD), ["F.radius.#0", "F.radius.#1", "F.factor.#0", "plain"])

    def test_objects_that_give_a_stem_different_numbers_in_other_order_differ(self):
        # planted defect: the two radius variables swap places (the second now comes first)
        swapped = [(0, 4096, "F.radius.205"), (4096, 4096, "F.radius.196"), (8192, 4096, "F.factor.216"), (12288, 8, "plain")]
        self.assertNotEqual(names(self.OLD), names(swapped))

    def test_a_reference_to_another_stem_differs(self):
        # planted defect: the variable that the old object calls F.factor.* is called F.radius.* in the new one
        other = [(0, 4096, "F.radius.196"), (4096, 4096, "F.radius.205"), (8192, 4096, "F.radius.216"), (12288, 8, "plain")]
        self.assertNotEqual(names(self.OLD), names(other))

    def test_a_missing_variable_changes_the_ranks_or_the_names(self):
        # planted defect: one of the radius variables is gone from the new object
        fewer = [(0, 4096, "F.radius.196"), (8192, 4096, "F.factor.216"), (12288, 8, "plain")]
        self.assertNotEqual(names(self.OLD), names(fewer))

    def test_plain_names_and_other_sections_are_untouched(self):
        syms = sbc.canonical_data_names({".bss": [(0, 4, "counter"), (4, 4, "g.x.7")], ".data": [(0, 4, "g.x.3")]})
        self.assertEqual(syms[".bss"], [(0, 4, "counter"), (4, 4, "g.x.#0")])
        self.assertEqual(syms[".data"], [(0, 4, "g.x.#0")])


if __name__ == "__main__":
    unittest.main()
